UI api experiments (mostly)

# ION

had troubles with mixing C and ION. I'd like to avoid double-defining datastructures on both sides. How do I tell ION not to be lazy and generate the symbols that I'd like to use on the C side? #foreign has
to be eager somehow.

An example where this bit me: even with -fullgen, enumeration constants are not generated! So I have the structs alright, but I can't build my switch/case in C.

So it seems easy to consume a C library but hard to collaborate/exchange datatypes between a C library and a ion module. Maybe this shouldnt really work, but it almost worked (I.e. it worked as long as I include just enough ion modules in my program for it to work)

This lead to an unexpected surprise down the line.

In hindsight, you certainly don't want to maintain and write @foreign declarations for something that might change.
