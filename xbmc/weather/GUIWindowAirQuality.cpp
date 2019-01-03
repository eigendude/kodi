/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowAirQuality.h"
#include "guilib/WindowIDs.h"
//#include "GUIUserMessages.h"
//#include "LangInfo.h"
//#include "ServiceBroker.h"
//#include "utils/StringUtils.h"
//#include "utils/URIUtils.h"
//#include "utils/Variant.h"
//#include "weather/WeatherManager.h"

#define CONTROL_BTNREFRESH             2
#define CONTROL_SELECTLOCATION         3
#define CONTROL_LABELUPDATED          11

#define CONTROL_STATICTEMP           223
#define CONTROL_STATICFEEL           224
#define CONTROL_STATICUVID           225
#define CONTROL_STATICWIND           226
#define CONTROL_STATICDEWP           227
#define CONTROL_STATICHUMI           228

#define CONTROL_LABELD0DAY            31
#define CONTROL_LABELD0HI             32
#define CONTROL_LABELD0LOW            33
#define CONTROL_LABELD0GEN            34
#define CONTROL_IMAGED0IMG            35

#define LOCALIZED_TOKEN_FIRSTID      370
#define LOCALIZED_TOKEN_LASTID       395

CGUIWindowAirQuality::CGUIWindowAirQuality() :
  CGUIWindow(WINDOW_AIR_QUALITY, "MyAir.xml")
{
  // Initialize CGUIWindow
  m_loadType = KEEP_IN_MEMORY;
}

CGUIWindowAirQuality::~CGUIWindowAirQuality() = default;

void CGUIWindowAirQuality::OnInitWindow()
{
  UpdateButtons();
  CGUIWindow::OnInitWindow();
}

void CGUIWindowAirQuality::UpdateButtons()
{
  /*! @todo
  SET_CONTROL_LABEL(WEATHER_LABEL_LOCATION, CServiceBroker::GetWeatherManager().GetLocation(CServiceBroker::GetWeatherManager().GetArea()));
  SET_CONTROL_LABEL(CONTROL_LABELUPDATED, CServiceBroker::GetWeatherManager().GetLastUpdateTime());

  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_COND, CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_COND));
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_TEMP, CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_TEMP) + g_langInfo.GetTemperatureUnitString());
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_FEEL, CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_FEEL) + g_langInfo.GetTemperatureUnitString());
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_UVID, CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_UVID));
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_WIND, CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_WIND));
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_DEWP, CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_DEWP) + g_langInfo.GetTemperatureUnitString());
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_HUMI, CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_HUMI));
  SET_CONTROL_FILENAME(WEATHER_IMAGE_CURRENT_ICON, CServiceBroker::GetWeatherManager().GetInfo(WEATHER_IMAGE_CURRENT_ICON));

  //static labels
  SET_CONTROL_LABEL(CONTROL_STATICTEMP, 401);  //Temperature
  SET_CONTROL_LABEL(CONTROL_STATICFEEL, 402);  //Feels Like
  SET_CONTROL_LABEL(CONTROL_STATICUVID, 403);  //UV Index
  SET_CONTROL_LABEL(CONTROL_STATICWIND, 404);  //Wind
  SET_CONTROL_LABEL(CONTROL_STATICDEWP, 405);  //Dew Point
  SET_CONTROL_LABEL(CONTROL_STATICHUMI, 406);  //Humidity

  for (int i = 0; i < NUM_DAYS; i++)
  {
    SET_CONTROL_LABEL(CONTROL_LABELD0DAY + (i*10), CServiceBroker::GetWeatherManager().GetForecast(i).m_day);
    SET_CONTROL_LABEL(CONTROL_LABELD0HI + (i*10), CServiceBroker::GetWeatherManager().GetForecast(i).m_high + g_langInfo.GetTemperatureUnitString());
    SET_CONTROL_LABEL(CONTROL_LABELD0LOW + (i*10), CServiceBroker::GetWeatherManager().GetForecast(i).m_low + g_langInfo.GetTemperatureUnitString());
    SET_CONTROL_LABEL(CONTROL_LABELD0GEN + (i*10), CServiceBroker::GetWeatherManager().GetForecast(i).m_overview);
    SET_CONTROL_FILENAME(CONTROL_IMAGED0IMG + (i*10), CServiceBroker::GetWeatherManager().GetForecast(i).m_icon);
  }
  */
}

/*! @todo
void CGUIWindowAirQuality::SetProperties()
{
  // Current weather
  int iCurWeather = CServiceBroker::GetWeatherManager().GetArea();
  SetProperty("Location", CServiceBroker::GetWeatherManager().GetLocation(iCurWeather));
  SetProperty("LocationIndex", iCurWeather);
  SetProperty("Updated", CServiceBroker::GetWeatherManager().GetLastUpdateTime());
  SetProperty("Current.ConditionIcon", CServiceBroker::GetWeatherManager().GetInfo(WEATHER_IMAGE_CURRENT_ICON));
  SetProperty("Current.Condition", CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_COND));
  SetProperty("Current.Temperature", CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_TEMP));
  SetProperty("Current.FeelsLike", CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_FEEL));
  SetProperty("Current.UVIndex", CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_UVID));
  SetProperty("Current.Wind", CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_WIND));
  SetProperty("Current.DewPoint", CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_DEWP));
  SetProperty("Current.Humidity", CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_HUMI));
  // we use the icons code number for fanart as it's the safest way
  std::string fanartcode = URIUtils::GetFileName(CServiceBroker::GetWeatherManager().GetInfo(WEATHER_IMAGE_CURRENT_ICON));
  URIUtils::RemoveExtension(fanartcode);
  SetProperty("Current.FanartCode", fanartcode);

  // Future weather
  std::string day;
  for (int i = 0; i < NUM_DAYS; i++)
  {
    day = StringUtils::Format("Day%i.", i);
    SetProperty(day + "Title", CServiceBroker::GetWeatherManager().GetForecast(i).m_day);
    SetProperty(day + "HighTemp", CServiceBroker::GetWeatherManager().GetForecast(i).m_high);
    SetProperty(day + "LowTemp", CServiceBroker::GetWeatherManager().GetForecast(i).m_low);
    SetProperty(day + "Outlook", CServiceBroker::GetWeatherManager().GetForecast(i).m_overview);
    SetProperty(day + "OutlookIcon", CServiceBroker::GetWeatherManager().GetForecast(i).m_icon);
    fanartcode = URIUtils::GetFileName(CServiceBroker::GetWeatherManager().GetForecast(i).m_icon);
    URIUtils::RemoveExtension(fanartcode);
    SetProperty(day + "FanartCode", fanartcode);
  }
}

void CGUIWindowAirQuality::ClearProperties()
{
  // Current weather
  SetProperty("Location", "");
  SetProperty("LocationIndex", "");
  SetProperty("Updated", "");
  SetProperty("Current.ConditionIcon", "");
  SetProperty("Current.Condition", "");
  SetProperty("Current.Temperature", "");
  SetProperty("Current.FeelsLike", "");
  SetProperty("Current.UVIndex", "");
  SetProperty("Current.Wind", "");
  SetProperty("Current.DewPoint", "");
  SetProperty("Current.Humidity", "");
  SetProperty("Current.FanartCode", "");

  // Future weather
  std::string day;
  for (int i = 0; i < NUM_DAYS; i++)
  {
    day = StringUtils::Format("Day%i.", i);
    SetProperty(day + "Title", "");
    SetProperty(day + "HighTemp", "");
    SetProperty(day + "LowTemp", "");
    SetProperty(day + "Outlook", "");
    SetProperty(day + "OutlookIcon", "");
    SetProperty(day + "FanartCode", "");
  }
}
*/
