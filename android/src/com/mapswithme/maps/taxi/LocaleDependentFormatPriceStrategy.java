package com.mapswithme.maps.taxi;

import android.support.annotation.NonNull;

import com.mapswithme.util.Utils;

class LocaleDependentFormatPriceStrategy implements FormatPriceStrategy
{
  @NonNull
  @Override
  public String format(@NonNull TaxiInfo.Product product)
  {
    return Utils.formatCurrencyString(product.getPrice(), product.getCurrency());
  }
}
