#include "fct.h"
#include "../../src/c/api-client.h"

char MapsWithMe_Base64Char(int x);
int MapsWithMe_LatToInt(double lat, int maxValue);
double MapsWithMe_LonIn180180(double lon);
int MapsWithMe_LonToInt(double lon, int maxValue);
void MapsWithMe_LatLonToString(double lat, double lon, char * s, int nBytes);

static const int MAX_POINT_BYTES = 10;
static const int TEST_COORD_BYTES = 9;

char const * TestLatLonToStr(double lat, double lon)
{
  static char s[TEST_COORD_BYTES + 1] = {0};
  MapsWithMe_LatLonToString(lat, lon, s, TEST_COORD_BYTES);
  return s;
}

FCT_BGN()
{

  FCT_QTEST_BGN(MapsWithMe_Base64Char)
  {
    fct_chk_eq_int('A', MapsWithMe_Base64Char(0));
    fct_chk_eq_int('B', MapsWithMe_Base64Char(1));
    fct_chk_eq_int('9', MapsWithMe_Base64Char(61));
    fct_chk_eq_int('-', MapsWithMe_Base64Char(62));
    fct_chk_eq_int('_', MapsWithMe_Base64Char(63));
  }
  FCT_QTEST_END();


  FCT_QTEST_BGN(MapsWithMe_LatToInt_0)
  {
    fct_chk_eq_int(499, MapsWithMe_LatToInt(0, 998));
    fct_chk_eq_int(500, MapsWithMe_LatToInt(0, 999));
    fct_chk_eq_int(500, MapsWithMe_LatToInt(0, 1000));
    fct_chk_eq_int(501, MapsWithMe_LatToInt(0, 1001));
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LatToInt_NearOrGreater90)
  {
    fct_chk_eq_int(999, MapsWithMe_LatToInt(89.9, 1000));
    fct_chk_eq_int(1000, MapsWithMe_LatToInt(89.999999, 1000));
    fct_chk_eq_int(1000, MapsWithMe_LatToInt(90.0, 1000));
    fct_chk_eq_int(1000, MapsWithMe_LatToInt(90.1, 1000));
    fct_chk_eq_int(1000, MapsWithMe_LatToInt(100.0, 1000));
    fct_chk_eq_int(1000, MapsWithMe_LatToInt(180.0, 1000));
    fct_chk_eq_int(1000, MapsWithMe_LatToInt(350.0, 1000));
    fct_chk_eq_int(1000, MapsWithMe_LatToInt(360.0, 1000));
    fct_chk_eq_int(1000, MapsWithMe_LatToInt(370.0, 1000));
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LatToInt_NearOrLess_minus90)
  {
    fct_chk_eq_int(1, MapsWithMe_LatToInt(-89.9, 1000));
    fct_chk_eq_int(0, MapsWithMe_LatToInt(-89.999999, 1000));
    fct_chk_eq_int(0, MapsWithMe_LatToInt(-90.0, 1000));
    fct_chk_eq_int(0, MapsWithMe_LatToInt(-90.1, 1000));
    fct_chk_eq_int(0, MapsWithMe_LatToInt(-100.0, 1000));
    fct_chk_eq_int(0, MapsWithMe_LatToInt(-180.0, 1000));
    fct_chk_eq_int(0, MapsWithMe_LatToInt(-350.0, 1000));
    fct_chk_eq_int(0, MapsWithMe_LatToInt(-360.0, 1000));
    fct_chk_eq_int(0, MapsWithMe_LatToInt(-370.0, 1000));
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LatToInt_NearOrLess_Rounding)
  {
    fct_chk_eq_int(0, MapsWithMe_LatToInt(-90.0, 2));
    fct_chk_eq_int(0, MapsWithMe_LatToInt(-45.1, 2));
    fct_chk_eq_int(1, MapsWithMe_LatToInt(-45.0, 2));
    fct_chk_eq_int(1, MapsWithMe_LatToInt(0.0, 2));
    fct_chk_eq_int(1, MapsWithMe_LatToInt(44.9, 2));
    fct_chk_eq_int(2, MapsWithMe_LatToInt(45.0, 2));
    fct_chk_eq_int(2, MapsWithMe_LatToInt(90.0, 2));
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LonIn180180)
  {
    fct_chk_eq_dbl(   0.0, MapsWithMe_LonIn180180(0));
    fct_chk_eq_dbl(  20.0, MapsWithMe_LonIn180180(20));
    fct_chk_eq_dbl(  90.0, MapsWithMe_LonIn180180(90));
    fct_chk_eq_dbl( 179.0, MapsWithMe_LonIn180180(179));
    fct_chk_eq_dbl(-180.0, MapsWithMe_LonIn180180(180));
    fct_chk_eq_dbl(-180.0, MapsWithMe_LonIn180180(-180));
    fct_chk_eq_dbl(-179.0, MapsWithMe_LonIn180180(-179));
    fct_chk_eq_dbl( -20.0, MapsWithMe_LonIn180180(-20));

    fct_chk_eq_dbl(0.0, MapsWithMe_LonIn180180(360));
    fct_chk_eq_dbl(0.0, MapsWithMe_LonIn180180(720));
    fct_chk_eq_dbl(0.0, MapsWithMe_LonIn180180(-360));
    fct_chk_eq_dbl(0.0, MapsWithMe_LonIn180180(-720));

    fct_chk_eq_dbl( 179.0, MapsWithMe_LonIn180180(360 + 179));
    fct_chk_eq_dbl(-180.0, MapsWithMe_LonIn180180(360 + 180));
    fct_chk_eq_dbl(-180.0, MapsWithMe_LonIn180180(360 - 180));
    fct_chk_eq_dbl(-179.0, MapsWithMe_LonIn180180(360 - 179));
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LonToInt_NearOrLess_Rounding)
  {
    //     135   90  45
    //        \  |  /
    //         03333
    //  180    0\|/2
    //    -----0-o-2---- 0
    // -180    0/|\2
    //         11112
    //        /  |  \
    //    -135  -90 -45

    fct_chk_eq_int(0, MapsWithMe_LonToInt(-180.0, 3));
    fct_chk_eq_int(0, MapsWithMe_LonToInt(-135.1, 3));
    fct_chk_eq_int(1, MapsWithMe_LonToInt(-135.0, 3));
    fct_chk_eq_int(1, MapsWithMe_LonToInt( -90.0, 3));
    fct_chk_eq_int(1, MapsWithMe_LonToInt( -60.1, 3));
    fct_chk_eq_int(1, MapsWithMe_LonToInt( -45.1, 3));
    fct_chk_eq_int(2, MapsWithMe_LonToInt( -45.0, 3));
    fct_chk_eq_int(2, MapsWithMe_LonToInt(   0.0, 3));
    fct_chk_eq_int(2, MapsWithMe_LonToInt(  44.9, 3));
    fct_chk_eq_int(3, MapsWithMe_LonToInt(  45.0, 3));
    fct_chk_eq_int(3, MapsWithMe_LonToInt( 120.0, 3));
    fct_chk_eq_int(3, MapsWithMe_LonToInt( 134.9, 3));
    fct_chk_eq_int(0, MapsWithMe_LonToInt( 135.0, 3));
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LonToInt_0)
  {
    fct_chk_eq_int(499, MapsWithMe_LonToInt(0, 997));
    fct_chk_eq_int(500, MapsWithMe_LonToInt(0, 998));
    fct_chk_eq_int(500, MapsWithMe_LonToInt(0, 999));
    fct_chk_eq_int(501, MapsWithMe_LonToInt(0, 1000));
    fct_chk_eq_int(501, MapsWithMe_LonToInt(0, 1001));

    fct_chk_eq_int(499, MapsWithMe_LonToInt(360, 997));
    fct_chk_eq_int(500, MapsWithMe_LonToInt(360, 998));
    fct_chk_eq_int(500, MapsWithMe_LonToInt(360, 999));
    fct_chk_eq_int(501, MapsWithMe_LonToInt(360, 1000));
    fct_chk_eq_int(501, MapsWithMe_LonToInt(360, 1001));

    fct_chk_eq_int(499, MapsWithMe_LonToInt(-360, 997));
    fct_chk_eq_int(500, MapsWithMe_LonToInt(-360, 998));
    fct_chk_eq_int(500, MapsWithMe_LonToInt(-360, 999));
    fct_chk_eq_int(501, MapsWithMe_LonToInt(-360, 1000));
    fct_chk_eq_int(501, MapsWithMe_LonToInt(-360, 1001));
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LonToInt_180)
  {
    fct_chk_eq_int(0, MapsWithMe_LonToInt(-180, 1000));
    fct_chk_eq_int(0, MapsWithMe_LonToInt(180, 1000));
    fct_chk_eq_int(0, MapsWithMe_LonToInt(-180 - 360, 1000));
    fct_chk_eq_int(0, MapsWithMe_LonToInt(180 + 360, 1000));
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LonToInt_360)
  {
    fct_chk_eq_int(2, MapsWithMe_LonToInt(0, 3));
    fct_chk_eq_int(2, MapsWithMe_LonToInt(0 + 360, 3));
    fct_chk_eq_int(2, MapsWithMe_LonToInt(0 - 360, 3));

    fct_chk_eq_int(2, MapsWithMe_LonToInt(1, 3));
    fct_chk_eq_int(2, MapsWithMe_LonToInt(1 + 360, 3));
    fct_chk_eq_int(2, MapsWithMe_LonToInt(1 - 360, 3));

    fct_chk_eq_int(2, MapsWithMe_LonToInt(-1, 3));
    fct_chk_eq_int(2, MapsWithMe_LonToInt(-1 + 360, 3));
    fct_chk_eq_int(2, MapsWithMe_LonToInt(-1 - 360, 3));
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LatLonToString)
  {
    fct_chk_eq_str("AAAAAAAAA", TestLatLonToStr(-90, -180));
    fct_chk_eq_str("qqqqqqqqq", TestLatLonToStr(90, -180));
    fct_chk_eq_str("_________", TestLatLonToStr(90, 179.999999));
    fct_chk_eq_str("VVVVVVVVV", TestLatLonToStr(-90, 179.999999));
    fct_chk_eq_str("wAAAAAAAA", TestLatLonToStr(0.0, 0.0));
    fct_chk_eq_str("P________", TestLatLonToStr(-0.000001, -0.000001));
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LatLonToString_PrefixIsTheSame)
  {
    for (double lat = -95; lat <= 95; lat += 0.7)
    {
      for (double lon = -190; lon < 190; lon += 0.9)
      {
        char prevStepS[MAX_POINT_BYTES + 1] = {0};
        MapsWithMe_LatLonToString(lat, lon, prevStepS, MAX_POINT_BYTES);

        for (int len = MAX_POINT_BYTES - 1; len > 0; --len)
        {
          // Test that the current string is a prefix of the previous one.
          char s[MAX_POINT_BYTES] = {0};
          MapsWithMe_LatLonToString(lat, lon, s, len);
          prevStepS[len] = 0;
          fct_chk_eq_str(s, prevStepS);
        }
      }
    }
  }
  FCT_QTEST_END();

  FCT_QTEST_BGN(MapsWithMe_LatLonToString_StringDensity)
  {
    int b64toI[256];
    for (int i = 0; i < 256; ++i) b64toI[i] = -1;
    for (int i = 0; i < 64; ++i) b64toI[MapsWithMe_Base64Char(i)] = i;

    int num1[256] = {0};
    int num2[256][256] = {0};

    for (double lat = -90; lat <= 90; lat += 0.1)
    {
      for (double lon = -180; lon < 180; lon += 0.05)
      {
        char s[3] = {0};
        MapsWithMe_LatLonToString(lat, lon, s, 2);
        ++num1[b64toI[s[0]]];
        ++num2[b64toI[s[0]]][b64toI[s[1]]];
       }
    }

    int min1 = 1 << 30, min2 = 1 << 30;
    int max1 = 0, max2 = 0;
    for (int i = 0; i < 256; ++i)
    {
      if (num1[i] != 0 && num1[i] < min1) min1 = num1[i];
      if (num1[i] != 0 && num1[i] > max1) max1 = num1[i];
      for (int j = 0; j < 256; ++j)
      {
        if (num2[i][j] != 0 && num2[i][j] < min2) min2 = num2[i][j];
        if (num2[i][j] != 0 && num2[i][j] > max2) max2 = num2[i][j];
      }
    }

    // printf("\n1: %i-%i   2: %i-%i\n", min1, max1, min2, max2);
    fct_chk((max1 - min1) * 1.0 / max1 < 0.05);
    fct_chk((max2 - min2) * 1.0 / max2 < 0.05);
  }
  FCT_QTEST_END();
}
FCT_END();
