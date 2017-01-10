
Some things I wish I knew before I started hacking on this project.
-------------------------------------------------------------------

This might also be useful to new users (since there is no
documentation yet).

The colors used in the visualisation indicate where that particular
data point originates from in the original file/data. Things from the
begining of the selection tend to be yellow and those from the end
blue.

Don't bother looking for the histogram generation algorithms. There
used to be one in C++ when the digram view was seperated but that is
no more. The histograms aren't computed, we rely on the graphics for
that. It's cleaner, nicer, and less work. We just draw all the points,
if there happen to be a lot it will be more visible.
