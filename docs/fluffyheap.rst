**********************
``struct fluffyheap*``
**********************

``struct fluffyheap*`` is the primary state where all
operations operated on

Since
*****
Version 0.1

Functions
*********

Static functions
################
+--------------------+-------------------------------------------+---------------------+
| Return value       | Function description                      | Link                |
+====================+===========================================+=====================+
| struct fh_context* | fh_attach_thread(struct fluffyheap* self) | `fh_attach_thread`_ |
+--------------------+-------------------------------------------+---------------------+
| void               | fh_detach_thread(struct fluffyheap* self) | `fh_detach_thread`_ |
+--------------------+-------------------------------------------+---------------------+

Constructor
###########
.. code-block:: c

   struct cuteheap* fh_new(struct fh_param* param)



