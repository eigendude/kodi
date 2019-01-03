set(SOURCES GUIWindowAirQuality.cpp
            GUIWindowWeather.cpp
            WeatherJob.cpp
            WeatherManager.cpp)

set(HEADERS GUIWindowAirQuality.h
            GUIWindowWeather.h
            WeatherJob.h
            WeatherManager.h)

core_add_library(weather)
