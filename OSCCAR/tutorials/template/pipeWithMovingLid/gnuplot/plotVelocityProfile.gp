reset

set terminal png
set output "../images/velocityProfile.png"

set xlabel "Ux (m/s)"
set ylabel "y (m)"

pp = '../postProcessing/sets'

plot \
  pp."/0.02/sampleLine_U.xy" using 2:1 with lines title "0.1",\
  pp."/0.04/sampleLine_U.xy" using 2:1 with lines title "0.2",\
  pp."/0.06/sampleLine_U.xy" using 2:1 with lines title "0.3",\
  pp."/0.08/sampleLine_U.xy" using 2:1 with lines title "0.4",\
  pp."/0.1/sampleLine_U.xy" using 2:1 with lines title "0.5"
