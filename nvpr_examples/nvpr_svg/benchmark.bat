mkdir results
.\gs_bezier.exe -xbenchmark -forceVBUM -16 -best -dlist > results\result16s.txt
.\gs_bezier.exe -xbenchmark -forceVBUM -8 -best -dlist > results\result8s.txt
.\gs_bezier.exe -xbenchmark -forceVBUM -4 -best -dlist > results\result4s.txt
.\gs_bezier.exe -xbenchmark -forceVBUM -2 -best -dlist > results\result2s.txt
.\gs_bezier.exe -xbenchmark -forceVBUM -16 -dlist > results\result16.txt
.\gs_bezier.exe -xbenchmark -forceVBUM -8 -dlist > results\result8.txt
.\gs_bezier.exe -xbenchmark -forceVBUM -4 -dlist > results\result4.txt
.\gs_bezier.exe -xbenchmark -forceVBUM -2 -dlist > results\result2.txt
.\gs_bezier.exe -xbenchmark -forceVBUM -1 -dlist > results\result1.txt
