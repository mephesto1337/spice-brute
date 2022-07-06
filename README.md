# spice-brute

More like a proof of concept to bruteforce spice sessions' password.
In an ideal world, this tool should be integrated to
[hydra](https://github.com/vanhauser-thc/thc-hydra).


## How to compile

### Dependencies
This tool depends on `glib-2.0` and `spice-client-glib-2.0`

#### Ubuntu/Debian
```bash
apt install clang make libspice-client-glib-2.0-dev libglib2.0-dev
```

#### Fedora
```bash
dnf install clang make glib2-devel spice-glib-devel
```

#### Archlinux
```bash
pacman -S clang make glib2 spice-gtk gobject-introspection pkg-config \
  spice-protocol
```
