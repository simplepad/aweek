# Aweek

## Description
CLI program to track new episodes for weekly airing anime.
Supports adding, deleting, editing anime, as well as quickly updating downloaded episodes count, marking anime as ignored, or setting certain episodes as delayed.

Can print new episodes' information in a pretty or easy to parse way - handy for scripting or integrating into configurable toolbars (AwesomeWM's wibox, Polybar and so on).

## Prerequisites
`json-c`

## Build
* **Debug**
```sh
make
```

* **Release**
```sh
DEBUG=false make
```