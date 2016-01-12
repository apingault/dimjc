# dimjc
DIM (CERN) job control


# To install :
# dim installed in /usr/local/dim

# With scons :
sudo mkdir -p /opt/dhcal/lib

sudo mkdir -p /opt/dhcal/bin

sudo scons install

# With cmake :
mkdir build

cd build

cmake -DDIMDIR=/path/to/dim ..

make install

# modify etc/dimjcd according to your installation (DIM_DNS_NODE)
sudo cp etc/dimjcd /etc/init.d/dimjcd

# Start on server :
sudo /etc/init.d/dimjcd start

# On client side :
The DimJobInterface class is provided to start/stop and query job status.

Example :

DimJobInterface *pInterface = new DimJibInterface();

pInterface->loadJSON("my_setup.json");

pInterface->startJobs("localhost");

pInterface->startJob("localhost", "MyApplication");
