# dimjcdqm
DIM (CERN) job control

# To install :
# dim installed in /usr/local/dim

# With scons :
``` bash
sudo mkdir -p /opt/dhcal/lib
sudo mkdir -p /opt/dhcal/bin
sudo scons install
```

# With cmake :
``` bash
mkdir build
cd build
cmake -DDIMDIR=/path/to/dim ..
make install
```

# copy etc/dimjcdqmd to /etc/init.d
``` bash
sudo cp etc/dimjcdqmd /etc/init.d/dimjcdqmd
```

# modify etc/dqm4hepemvExample according to your installation DIM_DNS_NODE) then copy it to /etc/default/dqm4hepenv
``` bash
cp /etc/dqm4hepemvExample /etc/default/dqm4hepenv
```
# Start on server :
``` bash
sudo /etc/init.d/dimjcdqmd start
```

# On client side :
The DimDQMJobInterface class is provided to start/stop and query job status.

Example :
``` c++
DimDQMJobInterface *pInterface = new DimDQMJobInterface();
pInterface->loadJSON("my_setup.json");
pInterface->startJobs("localhost");
pInterface->startJob("localhost", "MyApplication");
```