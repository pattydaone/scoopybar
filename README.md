# Scoopybar 

This project is currently under development, and knowing me, may never get finished. It is, at the moment, so early as to be entirely 
unusable.

In a broad sense, it intends to be a wayland port of the MacOs status bar "SketchyBar", which has some design decisions which I prefer to 
bars such as waybar:

- Every aspect of the bar can be changed at runtime

- The bar is not monolithic; this means that all the bar really does is display text/simple widgets. The actual logic must be scripted or coded 
  by the user. This script will send messages to the bar, telling it to update the widget.

- Aesthetic configuration is rather simple, and done in the config file. This does not aim to be a ricer's bar. If that's what you want, then use 
  waybar. Personally, I find gtk css to be exceptionally irritating.

The non-monolithic nature of the bar aims to reduce burden on me (the maintainer of the bar), and also allow people to create, share, and edit widgets 
easily and without having to recompile the entire bar.

I am taking great reference from "yambar" for the wayland backend, as this bar is implemented using wayland/wlr protocol as opposed to using an abstraction 
(such as gtk). **Thus far** I have not taken any code from there.

# To work on continuously

- Error checking
- Logging

# TODO list

- [ ] Scale stuff properly
- [ ] Extra configuration testing
- [ ] Add basic items
- [ ] Figure out text rendering
- [ ] Configuration for text items

# Done

having a list of things ive completed helps keep me sane: no matter how bad things look, i can look back at all i have done and see how far ive come indeed

- [X] Add multi-monitor support
- [X] Add some error checking
- [X] Logging
- [X] "Improve" logging API with printf-like formatting
- [X] .ini file parser
- [X] Figure out how I intend to attach buffers (using pixman)
- [X] Add double buffering with two separate buffers
- [X] Add wl_callbacks to handle rendering those buffers
- [X] Figure out how to actually write to those buffers via pixman
- [X] Have bar follow workspace changes
- [X] Configuration for bar
- [X] Add special logging for configuration errors/warnings/etc. (should include config file _line_)
# The design

## Configuration 

Typical unix-like configuration:

```
[bar]

// global configuration options

[item]

// type, location, script, aesthetics...

[item]

// next item... 

...
```

## Item types

- text

- graph

- slider

- group

- pop-up menu

## Bar Hierarchy
```
Bar
├── Item1
│   └── Script to call
│
├── Item2
│   └── Group
│       ├── Sub-Item1
│       ├── Sub-Item2
│       └── Sub-Item2
│
└── Item3
```
## IPC model

### Shell scripts

The binary, scoopybar, will have the capability to send messages to the main bar. Thus, a shell script for an item will do its logic, then invoke scoopybar and send the message.

### c/c++ binaries

While one of these binaries could employ the above tactic, there's probably a better way. Here are my two rough ideas:

- When the bar runs the binary, it opens a pipe between the sub-process and itself, thus allowing the forked process to send messages through this pipe.

- The bar has a persistent socket/linux message queue open which any binary can send messages to. This is the more likely option, as the binary also needs to be able to send messages to the bar. What if there are multiple bars? A bar, or multiple, having its own socket with which anyone can interface makes the most sense, I think.

