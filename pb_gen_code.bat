cd .\protocol

.\protoc.exe --cpp_out=.\ db_data.proto

.\protoc.exe --cpp_out=.\ server_base.proto
.\protoc.exe --cpp_out=.\ server_common.proto

.\protoc.exe --cpp_out=.\ server_db.proto


.\protoc.exe --cpp_out=.\ client_base.proto
.\protoc.exe --cpp_out=.\ client_common.proto
