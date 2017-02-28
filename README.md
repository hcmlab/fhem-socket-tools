Introduction
-----------------------------------

These are two command line tools that help connect to a telnet server, enter a password, and submit a command afterwards. A password is optional and considered only if given. socktool connects to unsecured servers, thus should *only* be used for connections to localhost. sockssl connects to a ssl/tls secured telnet server.


License
-----------------------------------
These tools are available as open source and licensed under the [Eclipse Public License - Version 1.0](LICENSE.html).
<br><br>


Usage
-----------------------------------
**1.** Open a terminal (bash) to/at the RaspBerry (e.g. SSH)
<br><br>

**2.** Download the project
```bash
git clone https://hcm-lab.de/git/iot/fhem-socket-tools.git
cd fhem-socket-tools
```

**3.** Build the tools<br>
```bash
./build.sh
```
<br>


**4.1.** SSL socket
```bash
./sockssl IP Port Password "Command"
```

**4.1.1.** SSL socket / Show verbose output
```bash
./sockssl IP Port Password "Command" 1
```


**4.2.** Unsecured socket
```bash
./socktool 127.0.0.1 Port Password "Command"
```

**4.2.1.** Unsecured socket / Show verbose output
```bash
./socktool 127.0.0.1 Port Password "Command" 1
```
<br><br>





