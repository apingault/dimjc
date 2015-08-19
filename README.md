# dimjc
DIM (CERN) job control

# To install
# dim installed in /usr/local/dim

sudo mkdir -p /opt/dhcal/lib
sudo mkdir -p /opt/dhcal/bin

sudo scons install

# modify etc/dimjcd according to your installation (DIM_DNS_NODE)

sudo cp etc/dimjcd /etc/init.d/dimjcd


# Start

sudo /etc/init.d/dimjcd start