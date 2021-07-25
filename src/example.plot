set xlabel 'time'

set yrange [0:]

set xdata time
# every 20 seconds
set xtics 20
set timefmt "%s"

set grid

set key below height 1

set terminal pngcairo size 800,500

set title 'cpu usage'
set ylabel 'usage [%]'
set output "example_cpu.png"
# time is in milliseconds
plot 'cpu_usage_all.rrd' using ($1/1000):2 with linespoints lw 2 lt rgb "violet" title "usage, all", \
     'cpu_usage_avg.rrd' using ($1/1000):2 with linespoints lw 2 lt rgb "yellow" title "usage, 5s avg", \
     'cpu_usage_max.rrd' using ($1/1000):2 with linespoints lw 2 lt rgb "red" title "usage, 5s max", \
     'cpu_usage_min.rrd' using ($1/1000):2 with linespoints lw 2 lt rgb "green" title "usage, 5s min", \

set title 'memory usage'
set ylabel 'memory [GB]'
set output "example_mem.png"
# time is in milliseconds, memory is in kB
plot 'mem_available_all.rrd' using ($1/1000):($2/1024**2) with linespoints lw 2 title "available, all", \
     'mem_available_avg.rrd' using ($1/1000):($2/1024**2) with linespoints lw 2 title "available, 5s avg", \
     'mem_available_max.rrd' using ($1/1000):($2/1024**2) with linespoints lw 2 title "available, 5s max", \
     'mem_available_min.rrd' using ($1/1000):($2/1024**2) with linespoints lw 2 title "available, 5s min", \
     'mem_cached_all.rrd'    using ($1/1000):($2/1024**2) with linespoints lw 2 title "cached, all", \
     'mem_cached_avg.rrd'    using ($1/1000):($2/1024**2) with linespoints lw 2 title "cached, 5s avg", \
     'mem_cached_max.rrd'    using ($1/1000):($2/1024**2) with linespoints lw 2 title "cached, 5s max", \
     'mem_cached_min.rrd'    using ($1/1000):($2/1024**2) with linespoints lw 2 title "cached, 5s min", \
     'mem_buffers_all.rrd'   using ($1/1000):($2/1024**2) with linespoints lw 2 title "buffers, all", \
     'mem_buffers_avg.rrd'   using ($1/1000):($2/1024**2) with linespoints lw 2 title "buffers, 5s avg", \
     'mem_buffers_max.rrd'   using ($1/1000):($2/1024**2) with linespoints lw 2 title "buffers, 5s max", \
     'mem_buffers_min.rrd'   using ($1/1000):($2/1024**2) with linespoints lw 2 title "buffers, 5s min"
