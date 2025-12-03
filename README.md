# Deepin System Monitor

Deepin system monitor: a more user-friendly system monitor.

Thanks a lot for [Lily Rivers](https://github.com/VioletDarkKitty/system-monitor)'s working, this project used a lot of his code.

## Features

### Core Features
- **Process Management**: View, search, and manage running processes
- **CPU Monitoring**: Real-time CPU usage with per-core breakdown
- **Memory Monitoring**: RAM and swap usage visualization
- **Network Monitoring**: Network traffic with per-application breakdown
- **Disk Monitoring**: Disk I/O read/write speeds
- **GPU Monitoring**: GPU usage and memory (if supported)
- **Temperature Monitoring**: CPU and system temperatures

### Extended Features (Ubuntu Compatible Branch)

#### Performance History
Real-time graphs showing historical data for:
- CPU usage (%)
- Memory usage (%)
- Network download/upload (KB/s)
- Disk read/write (KB/s)

System information panel displaying:
- Process count, thread count, handle count
- System uptime
- CPU speed (current and base frequency)
- CPU cache sizes (L1/L2/L3)

#### Disk Usage Analyzer
- Visual disk space analysis
- Directory tree with size breakdown
- File size distribution
- Threaded scanning for large directories

#### Systemd Services Manager
- View all systemd services
- Start/Stop/Restart/Enable/Disable services
- View service status and logs
- Filter by service state

#### Startup Applications Manager
- View autostart applications
- Enable/Disable startup entries
- Add new startup applications
- Support for XDG autostart directories

#### Docker Containers Monitor
- List running and stopped containers
- Start/Stop/Restart containers
- View container logs
- Monitor container resource usage

#### Alert Settings
- Configure CPU/Memory/Disk usage alerts
- Customizable thresholds
- Desktop notifications

#### Process Resource Limiting
- Set CPU limits using cgroups
- Set memory limits
- Apply nice values for priority

### Theme Support
All dialogs fully support dark and light themes with automatic switching.

## Dependencies

```bash
sudo apt install libpcap-dev libncurses5-dev libprocps-dev libxtst-dev libxcb-util0-dev
```

## Installation

```bash
mkdir build
cd build
qmake ..
make
sudo setcap cap_kill,cap_net_raw,cap_dac_read_search,cap_sys_ptrace+ep ./deepin-system-monitor
```

## Usage

```bash
./deepin-system-monitor
```

## Config file

```
~/.config/deepin/deepin-system-monitor/config.conf
```

## Authors

- **Deepin, Inc.** - Original authors (2011-2018)
- **[Lily Rivers](https://github.com/VioletDarkKitty)** - system-monitor base code
- **[Nic69Han](https://github.com/Nic69Han)** - Ubuntu compatibility, extended features, theme support

## Getting help

Any usage issues can ask for help via

* [Gitter](https://gitter.im/orgs/linuxdeepin/rooms)
* [IRC channel](https://webchat.freenode.net/?channels=deepin)
* [Forum](https://bbs.deepin.org)
* [WiKi](http://wiki.deepin.org/)

## Getting involved

We encourage you to report issues and contribute changes

* [Contribution guide for users](http://wiki.deepin.org/index.php?title=Contribution_Guidelines_for_Users)
* [Contribution guide for developers](http://wiki.deepin.org/index.php?title=Contribution_Guidelines_for_Developers).

## License

Deepin System Monitor is licensed under [GPLv3](LICENSE).
