#include "routing/geometry.hpp"

#include "routing/city_roads.hpp"
#include "routing/maxspeeds.hpp"
#include "routing/routing_exceptions.hpp"

#include "indexer/altitude_loader.hpp"
#include "indexer/data_source.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <string>

namespace routing
{
using namespace std;

double CalcFerryDurationHours(string const & durationHours, double roadLenKm)
{
  // Look for more info: https://confluence.mail.ru/display/MAPSME/Ferries
  // Shortly: the coefs were received from statistic about ferries with durations in OSM.
  double constexpr kIntercept = 0.2490726747447476;
  double constexpr kSlope = 0.02078913;

  if (durationHours.empty())
    return kIntercept + kSlope * roadLenKm;

  double durationH = 0.0;
  CHECK(strings::to_double(durationHours.c_str(), durationH), (durationHours));

  // See: https://confluence.mail.ru/download/attachments/249123157/image2019-8-22_16-15-53.png
  // Shortly: we drop some points: (x: lengthKm, y: durationH), that are upper or lower these two
  // lines.
  double constexpr kUpperBoundIntercept = 4.0;
  double constexpr kUpperBoundSlope = 0.037;
  if (kUpperBoundIntercept + kUpperBoundSlope * roadLenKm - durationH < 0)
    return kIntercept + kSlope * roadLenKm;

  double constexpr kLowerBoundIntercept = -2.0;
  double constexpr kLowerBoundSlope = 0.015;
  if (kLowerBoundIntercept + kLowerBoundSlope * roadLenKm - durationH > 0)
    return kIntercept + kSlope * roadLenKm;

  return durationH;
}

class RoadAttrsGetter
{
public:
  void Load(FilesContainerR const & cont)
  {
    try
    {
      if (cont.IsExist(CITY_ROADS_FILE_TAG))
        m_cityRoads.Load(cont.GetReader(CITY_ROADS_FILE_TAG));

      if (cont.IsExist(MAXSPEEDS_FILE_TAG))
        m_maxSpeeds.Load(cont.GetReader(MAXSPEEDS_FILE_TAG));
    }
    catch (Reader::Exception const & e)
    {
      LOG(LERROR, ("File", cont.GetFileName(), "Error while reading", CITY_ROADS_FILE_TAG, "or",
                   MAXSPEEDS_FILE_TAG, "section.", e.Msg()));
    }
  }

public:
  Maxspeeds m_maxSpeeds;
  CityRoads m_cityRoads;
};

// GeometryLoaderImpl ------------------------------------------------------------------------------
class GeometryLoaderImpl final : public GeometryLoader
{
public:
  GeometryLoaderImpl(DataSource const & dataSource, MwmSet::MwmHandle const & handle,
                     VehicleModelPtrT const & vehicleModel, bool loadAltitudes);

  // GeometryLoader overrides:
  void Load(uint32_t featureId, RoadGeometry & road) override;

private:
  VehicleModelPtrT m_vehicleModel;
  RoadAttrsGetter m_attrsGetter;
  FeaturesLoaderGuard m_guard;
  string const m_country;
  feature::AltitudeLoaderBase m_altitudeLoader;
  bool const m_loadAltitudes;
};

GeometryLoaderImpl::GeometryLoaderImpl(DataSource const & dataSource,
                                       MwmSet::MwmHandle const & handle,
                                       VehicleModelPtrT const & vehicleModel,
                                       bool loadAltitudes)
  : m_vehicleModel(move(vehicleModel))
  , m_guard(dataSource, handle.GetId())
  , m_country(handle.GetInfo()->GetCountryName())
  , m_altitudeLoader(dataSource, handle.GetId())
  , m_loadAltitudes(loadAltitudes)
{
  CHECK(handle.IsAlive(), ());
  CHECK(m_vehicleModel, ());

  m_attrsGetter.Load(handle.GetValue()->m_cont);
}

void GeometryLoaderImpl::Load(uint32_t featureId, RoadGeometry & road)
{
  auto feature = m_guard.GetFeatureByIndex(featureId);
  if (!feature)
    MYTHROW(RoutingException, ("Feature", featureId, "not found in", m_country));
  ASSERT_EQUAL(feature->GetID().m_index, featureId, ());

  feature->ParseGeometry(FeatureType::BEST_GEOMETRY);

  geometry::Altitudes altitudes;
  if (m_loadAltitudes)
    altitudes = m_altitudeLoader.GetAltitudes(featureId, feature->GetPointsCount());

  road.Load(*m_vehicleModel, *feature, altitudes.empty() ? nullptr : &altitudes, m_attrsGetter);
}

// FileGeometryLoader ------------------------------------------------------------------------------
class FileGeometryLoader final : public GeometryLoader
{
public:
  FileGeometryLoader(string const & fileName, VehicleModelPtrT const & vehicleModel);

  // GeometryLoader overrides:
  void Load(uint32_t featureId, RoadGeometry & road) override;

private:
  FeaturesVectorTest m_featuresVector;
  RoadAttrsGetter m_attrsGetter;
  shared_ptr<VehicleModelInterface> m_vehicleModel;
};

FileGeometryLoader::FileGeometryLoader(string const & fileName, VehicleModelPtrT const & vehicleModel)
  : m_featuresVector(fileName)
  , m_vehicleModel(vehicleModel)
{
  CHECK(m_vehicleModel, ());

  m_attrsGetter.Load(m_featuresVector.GetContainer());
}

void FileGeometryLoader::Load(uint32_t featureId, RoadGeometry & road)
{
  auto feature = m_featuresVector.GetVector().GetByIndex(featureId);
  CHECK(feature, ());
  feature->ParseGeometry(FeatureType::BEST_GEOMETRY);
  // Note. If FileGeometryLoader is used for generation cross mwm section for bicycle or
  // pedestrian routing |altitudes| should be used here.
  road.Load(*m_vehicleModel, *feature, nullptr /* altitudes */, m_attrsGetter);
}

// RoadGeometry ------------------------------------------------------------------------------------
RoadGeometry::RoadGeometry(bool oneWay, double weightSpeedKMpH, double etaSpeedKMpH,
                           Points const & points)
  : m_forwardSpeed{weightSpeedKMpH, etaSpeedKMpH}, m_backwardSpeed(m_forwardSpeed)
  , m_isOneWay(oneWay), m_valid(true), m_isPassThroughAllowed(false), m_inCity(false)
{
  ASSERT_GREATER(weightSpeedKMpH, 0.0, ());
  ASSERT_GREATER(etaSpeedKMpH, 0.0, ());

  m_junctions.reserve(points.size());
  for (auto const & point : points)
    m_junctions.emplace_back(mercator::ToLatLon(point), geometry::kDefaultAltitudeMeters);
}

void RoadGeometry::Load(VehicleModelInterface const & vehicleModel, FeatureType & feature,
                        geometry::Altitudes const * altitudes, RoadAttrsGetter & attrs)
{
  CHECK(altitudes == nullptr || altitudes->size() == feature.GetPointsCount(), ());

  m_highwayType = vehicleModel.GetHighwayType(feature);

  m_valid = vehicleModel.IsRoad(feature);
  m_isOneWay = vehicleModel.IsOneWay(feature);
  m_isPassThroughAllowed = vehicleModel.IsPassThroughAllowed(feature);

  uint32_t const fID = feature.GetID().m_index;
  m_inCity = attrs.m_cityRoads.IsCityRoad(fID);
  Maxspeed const maxSpeed = attrs.m_maxSpeeds.GetMaxspeed(fID);
  m_forwardSpeed = vehicleModel.GetSpeed(feature, {true /* forward */, m_inCity, maxSpeed});
  m_backwardSpeed = vehicleModel.GetSpeed(feature, {false /* forward */, m_inCity, maxSpeed});

  feature::TypesHolder types(feature);
  auto const & optionsClassfier = RoutingOptionsClassifier::Instance();
  for (uint32_t type : types)
  {
    if (auto const it = optionsClassfier.Get(type))
      m_routingOptions.Add(*it);
  }

  m_junctions.clear();
  m_junctions.reserve(feature.GetPointsCount());
  for (size_t i = 0; i < feature.GetPointsCount(); ++i)
  {
    m_junctions.emplace_back(mercator::ToLatLon(feature.GetPoint(i)),
                             altitudes ? (*altitudes)[i] : geometry::kDefaultAltitudeMeters);
  }

  if (m_routingOptions.Has(RoutingOptions::Road::Ferry))
  {
    auto const durationHours = feature.GetMetadata(feature::Metadata::FMD_DURATION);
    auto const roadLenKm = GetRoadLengthM() / 1000.0;
    double const durationH = CalcFerryDurationHours(durationHours, roadLenKm);

    CHECK(!base::AlmostEqualAbs(durationH, 0.0, 1e-5), (durationH));

    if (roadLenKm != 0.0)
    {
      m_forwardSpeed = m_backwardSpeed =
          SpeedKMpH(std::min(vehicleModel.GetMaxWeightSpeed(), roadLenKm / durationH));
    }
  }

  if (m_valid && (!m_forwardSpeed.IsValid() || !m_backwardSpeed.IsValid()))
  {
    auto const & id = feature.GetID();
    CHECK(!m_junctions.empty(), ("mwm:", id.GetMwmName(), ", featureId:", id.m_index));
    auto const & begin = m_junctions.front().GetLatLon();
    auto const & end = m_junctions.back().GetLatLon();
    LOG(LERROR,
        ("Invalid speed m_forwardSpeed:", m_forwardSpeed, "m_backwardSpeed:", m_backwardSpeed,
         "mwm:", id.GetMwmName(), ", featureId:", id.m_index, ", begin:", begin, "end:", end));
    m_valid = false;
  }
}

SpeedKMpH const & RoadGeometry::GetSpeed(bool forward) const
{
  return forward ? m_forwardSpeed : m_backwardSpeed;
}

double RoadGeometry::GetRoadLengthM() const
{
  double lenM = 0.0;
  for (size_t i = 1; i < GetPointsCount(); ++i)
  {
    lenM += ms::DistanceOnEarth(m_junctions[i - 1].GetLatLon(), m_junctions[i].GetLatLon());
  }

  return lenM;
}

// Geometry ----------------------------------------------------------------------------------------
Geometry::Geometry(unique_ptr<GeometryLoader> loader, size_t roadsCacheSize)
  : m_loader(move(loader))
  , m_featureIdToRoad(make_unique<RoutingFifoCache>(
        roadsCacheSize,
        [this](uint32_t featureId, RoadGeometry & road) { m_loader->Load(featureId, road); }))
{
  CHECK(m_loader, ());
}

RoadGeometry const & Geometry::GetRoad(uint32_t featureId)
{
  ASSERT(m_featureIdToRoad, ());
  ASSERT(m_loader, ());

  return m_featureIdToRoad->GetValue(featureId);
}

// static
unique_ptr<GeometryLoader> GeometryLoader::Create(DataSource const & dataSource,
                                                  MwmSet::MwmHandle const & handle,
                                                  VehicleModelPtrT const & vehicleModel,
                                                  bool loadAltitudes)
{
  CHECK(handle.IsAlive(), ());
  return make_unique<GeometryLoaderImpl>(dataSource, handle, vehicleModel, loadAltitudes);
}

// static
unique_ptr<GeometryLoader> GeometryLoader::CreateFromFile(
    string const & fileName, VehicleModelPtrT const & vehicleModel)
{
  return make_unique<FileGeometryLoader>(fileName, vehicleModel);
}
}  // namespace routing
