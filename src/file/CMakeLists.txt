# Create a file "object" library
set(FILE_SRC
    structure.h

    # File blocks
    Block_content.cpp
    Block_content.hpp
    Block_data_reader.cpp
    Block_data_reader.hpp
    Block_data_writer.cpp
    Block_data_writer.hpp
    Block_session.cpp
    Block_session.hpp
    Block_templates.cpp
    Block_templates.hpp

    # C API wrapper
    file.cpp

    # I/O operations
    Io_request.cpp
    Io_request.hpp
    Io_sync.cpp
    Io_sync.hpp
    Io_async.cpp
    Io_async.hpp

    #
    File_base.hpp
    File_base.cpp
    File_exception.cpp
    File_exception.hpp
    File_reader.cpp
    File_reader.hpp
    File_writer.cpp
    File_writer.hpp
)

add_library(file_obj OBJECT ${FILE_SRC})

