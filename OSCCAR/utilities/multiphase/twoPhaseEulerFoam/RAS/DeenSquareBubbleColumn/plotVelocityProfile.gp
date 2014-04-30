# ---------------------------------------------------------------------------- #
#
# File:    plotVelocityProfile.gp
# Date:    Wed Apr 30 17:19:53 CEST 2014
# Author:  Gijsbert Wierink
#
# Description:
#   A basic gnuplot script to plot the velocity profile as sampled by the sample
#   utility in this case. The script can be run using:
#
#       gnuplot plotVelocityProfile.gp
#
#   Note that this script is run automatically by the Allrun script.
#
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  # 


reset

set terminal png enhanced
set output "VelocityProfile.png"

set xlabel "x/D (-)"
set xtics 0.1

set ylabel "U (m/s)"

plot \
  "postProcessing/sets/20/sampleLine_U.water.xy" \
  using ($1/0.15):4 with lines notitle

# ----------------------------------------------------------------- end-of-file
