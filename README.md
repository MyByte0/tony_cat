# server_cat  
  
It is a multi-platform server frame base on c++20.  
using libasio, glog, libformat(on gcc).  
wiki: https://github.com/MyByte0/tony_cat/wiki  
  
The linux environment on doker hub  
debian os for developing:  
https://hub.docker.com/repository/docker/mybyte0/dem_ser  
for examlpe:  
docker pull mybyte0/dem_ser:dev  
docker run -it --cap-add sys_ptrace -v /mnt/hgfs/proj/:/data/proj image_id /bin/bash  
  
For Windows use vcpkg  
  
vcpkg install asio:x64-windows   
vcpkg install fmt:x64-windows  
vcpkg install glog:x64-windows  
vcpkg install tinyxml2:x64-windows  
vcpkg install protobuf:x64-windows  
  
  
  
install python3 and openpyxl  
  
open the project with cmake   
