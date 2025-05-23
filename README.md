# AM

A small command line utility for quickly and easily peeking into archives and unpacking its content.

For now it uses https://github.com/libarchive/libarchive for viewing and extracting archives.


## Usage

List the content of *archive.tar.gz*
```
$ am archive.tar.gz
```

Extract *archive.tar.gz* into the current working directory
```
$ am -x archive.tar.gz
```
Extract *archive.tar.gz* into *~/Downloads*
```
$ am -x archive.tar.gz -o ~/Downloads
```
---

By default `am` extracts the content in an archive-named directory if
the top level of the archive contains several files to avoid vomitting
the files all over the current directory.

Same behaviour if the root folder in the archive isn't named the same as the archive.

This can be ommited by:
```
$ am -x archive.tar.gz -p
```



## Supported Formats

Everything that libarchive supports

https://github.com/libarchive/libarchive?tab=readme-ov-file#supported-formats


## Dependencies:

**Debian / Ubuntu**

```
sudo apt install gcc make pkg-config libarchive-dev
```


## Manual installation
```
git clone https://github.com/pgml/am
cd am;
make;
make install;
```

