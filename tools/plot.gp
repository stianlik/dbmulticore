set terminal eps
set output "results/plot.eps"
plot "plot.dat" using 1:2 lt 12 title 'Seconds' with linespoints
