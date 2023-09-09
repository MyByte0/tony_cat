# tony_cat  
  
It is a multi-platform server frame base on c++20.  
using libasio, glog, libformat, libmysql/rocksdb.  

The linux environment on docker hub  
debian os for developing:  
<https://hub.docker.com/repository/docker/mybyte0/tony_cat>  
  
for examlpe
fetch the docker image:

```
docker pull mybyte0/tony_cat:v0.35  
```

or

```
docker build . -f Dockerfile -t mybyte0/tony_cat:v0.35
```

create docker  

```
docker run -it --network=host --cap-add sys_ptrace -v /mnt/hgfs/proj/:/data/proj mybyte0/tony_cat:v0.35 /bin/bash  
```
  
For Windows use vcpkg  
  
vcpkg install asio:x64-windows  
vcpkg install fmt:x64-windows  
vcpkg install glog:x64-windows  
vcpkg install tinyxml2:x64-windows  
vcpkg install protobuf:x64-windows  
vcpkg install rocksdb:x64-windows  
vcpkg install libmysql:x64-windows  
  
install python3 and openpyxl  
  
open the project with cmake  
