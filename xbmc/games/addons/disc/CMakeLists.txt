set(SOURCES GameClientDiscModel.cpp
            GameClientDiscs.cpp
            GameClientDiscSync.cpp
            GameClientDiscTransport.cpp
            GameClientDiscXML.cpp
)

set(HEADERS GameClientDiscModel.h
            GameClientDiscs.h
            GameClientDiscSync.h
            GameClientDiscTransport.h
            GameClientDiscXML.h
)

core_add_library(gamediscs)
