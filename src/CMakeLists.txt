set(SOURCES
    main.cpp
    MainWindow.cpp
    SerialHandler.cpp
    MorseDecoder.cpp
    MorseTable.cpp
    ToneGenerator.cpp
)

set(HEADERS
    MainWindow.h
    SerialHandler.h
    MorseDecoder.h
    MorseTable.h
    ToneGenerator.h
)

add_executable(morse-decoder ${SOURCES} ${HEADERS})

target_link_libraries(morse-decoder
    Qt6::Core
    Qt6::Widgets
    Qt6::SerialPort
    Qt6::Multimedia
)

install(TARGETS morse-decoder DESTINATION bin)
