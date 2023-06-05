``fluffyheap*``
######################

``fluffyheap*`` is the primary state where all
operations operated on

Since
*****
Version 0.1

Methods
#######
+--------------+-------------------------------------------+---------------------+
| Return value | Function description                      | Link                |
+==============+===========================================+=====================+
| void         | fh_attach_thread(fluffyheap* self) | `fh_attach_thread`_ |
+--------------+-------------------------------------------+---------------------+
| void         | fh_detach_thread(fluffyheap* self) | `fh_detach_thread`_ |
+--------------+-------------------------------------------+---------------------+

Constructor Detail
##################
.. code-block:: c

   fluffyheap* fh_new(fh_param* param)

Destructor Detail
#################
.. code-block:: c

   void fh_free(fluffyheap* self)

.. warning::

   Do not call this while heap in use

Thread safety
*************
Thread Safe, as long as not in use by other thread

Method Details
##############

fh_attach_thread
****************
.. code-block:: c

   void fh_attach_thread(struct fluffyheap* self)

fh_detach_thread
****************
.. code-block:: c

   void fh_detach_thread(struct fluffyheap* self)

