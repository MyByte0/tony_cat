# tony_cat  
  
It is a multi-platform server frame on c++20.  
using libasio, glog, libformat, optional on libmysql/rocksdb.  
 
  
The linux environment on docker hub  
debian os for developing:  
https://hub.docker.com/repository/docker/mybyte0/tony_cat  
for examlpe:  
```  
docker build . -f Dockerfile -t mybyte0/tony_cat:v0.3  or  docker pull mybyte0/tony_cat:v0.3  
```  

```  
docker run -it --cap-add sys_ptrace -v /mnt/hgfs/proj:/data/proj tony_cat:v0.3 /bin/bash  
```  

For Windows use vcpkg  
  
vcpkg install asio:x64-windows   
vcpkg install fmt:x64-windows
vcpkg install glog:x64-windows
vcpkg install tinyxml2:x64-windows  
vcpkg install protobuf:x64-windows  

optional:  
vcpkg install libmysql:x64-windows  
vcpkg install rocksdb:x64-windows  
  

other tools:  
install python3 and openpyxl  
  
open the project with cmake  
use database mysql: -DUSE_MYSQL=ON  
use database rocksdb: -DUSE_ROCKSDB=ON  
  
get start with:  
https://github.com/MyByte0/tony_cat/wiki

please give advices on the issues site.