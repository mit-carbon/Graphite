GENERAL INFORMATION:

This code solves the same molecular dynamics N-body problem as the 
original Water code in SPLASH (which is called WATER-NSQUARED in 
SPLASH-2), but uses a different algorithm.  In particular, it imposes a 
3-d spatial data structure on the cubical domain, resulting in a 3-d 
grid of boxes.  Every box contains a linked list of the molecules 
currently in that box (in the current time-step).  The advantage of the 
spatial grid is that a process that owns a box in the grid has to look 
at only its neighboring boxes for molecules that might be within the 
cutoff radius from a molecule in the box it owns.  This makes the 
algorithm O(n) instead of O(n^2).   For small problems (upto several 
hundred to a couple of thousand molecules) the overhead of the spatial 
data structure is not justified and WATER-NSQUARED might solve the 
problem faster.  But for large systems this program is much better.  
That is why we provide both, since both small and large systems are 
interesting in this case. 

All access to molecules is through the boxes in the spatial grid, and 
these boxes are the units of partitioning (unlike WATER-NSQUARED,
in which molecules are the units of partitioning). 

RUNNING THE PROGRAM:

The program is run in exactly the same way as the WATER-NSQUARED
program, and the input file has exactly the same parameters 
(To see how to run the program, please see the comment at the top 
of the water.C file or run it as "WATER-SPATIAL -h".  The input file 
has 10 parameters, of which the ones you would normally change 
are the number of molecules and the number of processors.  The other
parameters should be left at their values in the supplied input file
in the normal case).  

The one tricky parameter in WATER-SPATIAL is the cutoff radius (the
last parameter in the input file).  If the cutoff radius parameter
is set to 0 in the input file, the program will compute it itself. 
However, the way the program computes it, it may be as large as
about 11 Angstrom. Basically, the program sets it to half the length
of the computational cube that encloses all particles, or 11 Angstrom,
whichever is smaller.  Since the size of a box in the spatial grid 
(such a box being the unit of partitioning) is greater than or equal 
to the cutoff radius in our implementation, this leads to a very small 
number of boxes, and hence a very small number of usable processors) 
when the the number of molecules is small.  People who use this program 
on multiprocessor simulators may not be able to run very large problems 
(i.e. might be able to go to 512 or 1000 particles or so, for example).  
For this reason, we recommend using a cutoff value of 6.2 (Angstrom) in 
the input file when simulating, and expressly saying that you did this 
in any results you present (a good thing to remember: the intermolecular 
distance is typically about 3.1 Angstrom. We would not make the cutoff 
radius much smaller than 6.2 Angstrom).  Note that using 6.2 Angstrom 
is not chemically the right thing to do (about 11 Angstrom is for most 
problems), and changes the computational characteristics somewhat as well.
When running on real machines, we recommend doing the chemically correct 
thing, which is setting the cutoff parameter to 0 in the input file and
letting the program compute it. 

Like in WATER-NSQUARED, the only compile-time option (ifdef) is one that 
says to change the input distribution.  The default input distribution of 
molecules arranges them on a cubical lattice.  For this, the number of 
molecules must be an integer cube (8, 27, 64, 343, 512 ...).  If one 
wants to use a non-cube number of molecules, one can ignore the lattice 
and use a random distribution of particles in a cubical space by invoking 
the -DRANDOM compile-time option (see file initia.C). Note that a random 
distribution does not make too much physical sense, since it does not 
preserve chemical intermolecular distance ranges. If you do not use the 
lattice but use -DRANDOM, please say so explicitly in any results you 
report.

The program reads random numbers, to compute initial velocities, from
a file called random.in in the current working directory.  It does
this rather than generate random numbers to facilitate repeatability
and comparability of experiments.  The supplied file random.in has 
enough numbers for 512 molecules.  If you need more, add more random 
numbers between -4.0 and +4.0 to the file.

BASE PROBLEM SIZE:

The base problem size for an upto-64 processor machine is 512 molecules.
For this number of molecules, you can use the input file provided. With
the cutoff radius of 6.2 Angstrom in the input file, this results in 64 
boxes and hence 64 usable processors (1 box per processor).  To use more 
processors or get better load balance, increase the number of molecules. 

DATA DISTRIBUTION:

Our "POSSIBLE ENHANCEMENT" comments in the source code tell where one
might want to distribute data and how.  Data distribution, however,
does not make much difference to performance on the Stanford DASH 
multiprocessor for this code. 
