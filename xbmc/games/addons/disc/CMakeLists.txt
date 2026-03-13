set(SOURCES GameClientDiscModel.cpp
            GameClientDiscM3U.cpp
            GameClientDiscMergeUtils.cpp
            GameClientDiscs.cpp
            GameClientDiscTransport.cpp
            GameClientDiscXML.cpp
)

set(HEADERS GameClientDiscModel.h
            GameClientDiscM3U.h
            GameClientDiscMergeUtils.h
            GameClientDiscs.h
            GameClientDiscTransport.h
            GameClientDiscXML.h
)

core_add_library(gamediscs)
