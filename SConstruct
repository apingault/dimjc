import sys,os,commands
import  os
import re
import sys
def SWIGSharedLibrary(env, library, sources, **args):
  swigre = re.compile('(.*).i')
  if env.WhereIs('swig') is None:
    sourcesbis = []
    for source in sources:
      cName = swigre.sub(r'\1_wrap.c', source)
      cppName = swigre.sub(r'\1_wrap.cc', source)
      if os.path.exists(cName):
        sourcesbis.append(cName)
      elif os.path.exists(cppName):
        sourcesbis.append(cppName)
      else:
        sourcesbis.append(source)
  else:
    sourcesbis = sources
 
  if 'SWIGFLAGS' in args:
    args['SWIGFLAGS'] += ['-python']
  else:
    args['SWIGFLAGS'] = ['-python'] + env['SWIGFLAGS']
  args['SHLIBPREFIX']=""
  if sys.version >= '2.5':
    args['SHLIBSUFFIX']=".pyd"
 
  cat=env.SharedLibrary(library, sourcesbis, **args)
  return cat
 


# environment
#print "----------------------------------------------"
Decider('MD5-timestamp')
XDAQ_ROOT="/opt/xdaq"
DIM_ROOT="/usr/local/dim"
DHCAL_ROOT=os.path.abspath("..")


Bit64=False
Bit64=os.uname()[4]=='x86_64'

#NetLink=DHCAL_ROOT+"/netLink"
#if ( not os.path.exists(NetLink)):
#  NetLink=os.getenv("HOME")+"/netLink"

kl=os.uname()[2].split(".")
platform="UBUNTU"
if (kl[len(kl)-1][0:3] == 'el5'):
    platform="SLC5"

if (kl[len(kl)-2][0:3] == 'el6'):
    platform="SLC6"

Arm=os.uname()[4]=='armv7l'

if Arm or platform=="UBUNTU":
  boostsystem='boost_system'
  boostthread='boost_thread'
else:
  boostsystem='boost_system-mt'
  boostthread='boost_thread-mt'


Use_Dim=True
#os.environ.has_key("DIM_DNS_NODE")

# includes
INCLUDES=['include',"/usr/include/boost141/"]

if (Use_Dim):
  INCLUDES.append(DIM_ROOT+"/dim")
 
INCLUDES.append(commands.getoutput("python -c 'import distutils.sysconfig as conf; print conf.get_python_inc()'"))

INCLUDES.append("/usr/include/jsoncpp")
INCLUDES.append("/usr/include/libxml2")



CPPFLAGS=["-pthread","-O2","-DLINUX", "-DREENTRANT" ,"-Dlinux"]

#Library ROOT + some of XDAQ + DB 

LIBRARIES=['pthread',  'm', 'stdc++',boostsystem,boostthread,'dim','jsoncpp']



#Library path XDAQ,DHCAL and ROOT + Python
if (Bit64):
	LIBRARY_PATHS=["/usr/lib64","/usr/local/lib",DIM_ROOT+"/linux"]
else:
  LIBRARY_PATHS=["/usr/lib","/usr/local/lib",DIM_ROOT+"/linux"]
LIBRARY_PATHS.append(commands.getoutput("python -c 'import distutils.sysconfig as conf; print conf.PREFIX'")+"/lib")

if Use_Dim:
   CPPFLAGS.append("-DUSE_DIM")



#link flags
LDFLAGS=["-fPIC","-dynamiclib"]


# SWIG
SWIGSF=["-c++","-classic"]

for i in INCLUDES:
    SWIGSF.append("-I"+i)
print SWIGSF

# Create the Environment
env = Environment(CPPPATH=INCLUDES,CPPFLAGS=CPPFLAGS,LINKFLAGS=LDFLAGS, LIBS=LIBRARIES,LIBPATH=LIBRARY_PATHS,SWIGFLAGS=SWIGSF)

#print "CC is:",env.subst('$CPPPATH')

env['BUILDERS']['PythonModule'] = SWIGSharedLibrary


# Library source
LIBRARY_SOURCES=Glob("#src/*.cc")

#Shared library
dimjc=env.SharedLibrary("#lib/dimjc",LIBRARY_SOURCES)



#Daemon 
EXE_LIBPATH=LIBRARY_PATHS
EXE_LIBPATH.append("#lib")
EXE_LIBS=LIBRARIES
EXE_LIBS.append("dimjc")
djc=env.Program("bin/dimjc.exe",source="src/djc.cxx",LIBPATH=EXE_LIBPATH,LIBS=EXE_LIBS)
#dcs=env.Program("bin/dimccc",source="src/dcc.cxx",LIBPATH=EXE_LIBPATH,LIBS=EXE_LIBS)
#dzups=env.Program("bin/dimzup",source="src/dzup.cxx",LIBPATH=EXE_LIBPATH,LIBS=EXE_LIBS)

#Python module
_dimjc=env.PythonModule('_Ldimjc', ['Ldimjc.i'],LIBPATH="#lib",LIBS="dimjc")
env.AddPostAction(_dimjc,Copy("/usr/lib/python2.7/dist-packages/Ldimjc.py","Ldimjc.py"))
env.Install("/opt/dhcal/lib",dimjc)
env.Install("/opt/dhcal/bin",[djc])
env.InstallAs(["/usr/lib/python2.7/dist-packages/_Ldimjc.so"],[_dimjc])
###env.Install("/opt/dhcal/lib",dimjc)
###env.Install("/opt/dhcal/include/readout",myinc)

env.Alias('install', ["/opt/dhcal/lib","/opt/dhcal/bin","/usr/lib/python2.7/dist-packages"])
Default([dimjc,djc,_dimjc])


