.. ParaView-Python documentation master file, created by
   sphinx-quickstart on Fri Feb  8 01:06:42 2013.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

ParaView's Python documentation!
===========================================

ParaView offers rich scripting support through Python. This support is available
as part of the ParaView client (paraview), an MPI-enabled batch application
(pvbatch), the ParaView python client (pvpython), or any other Python-enabled
application. Using Python, users and developers can gain access to the ParaView
visualization engine.

Main modules
==================

.. toctree::
   :maxdepth: 2

   quick-start

   The ParaView Python Package <paraview>
   simple <paraview.simple>
   servermanager <paraview.servermanager>
   algortihm <paraview.vtk.algorithms>

   coprocessing <paraview.coprocessing>
   benchmark <paraview.benchmark>

   Available readers, sources, writers, filters and animation cues <paraview.servermanager_proxies>

ParaViewWeb
==================

.. toctree::
   :maxdepth: 2

   Web Visualization application <paraview.web.pv_web_visualizer>
   File loader <paraview.web.pv_web_file_loader>
   Data Prober <paraview.web.pv_web_data_prober>
