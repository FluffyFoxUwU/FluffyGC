GC Roots
########

This where GC starts marking for live objects.

Functions
#########

+--------------+--------------------------------------+---------------+
| Return value | Functions                            | Link          |
+==============+======================================+===============+
| fh_object*   | fh_dup_ref(@Nullable fh_object* ref) | `fh_dup_ref`_ |
+--------------+--------------------------------------+---------------+
| void         | fh_del_ref(fh_object* ref)           | `fh_del_ref`_ |
+--------------+--------------------------------------+---------------+

Functions detail
################

fh_dup_ref
**********
.. code-block:: c

   @Nullable
   fh_object* fh_dup_ref(@Nullable fh_object* ref)

Since
=====
Version 0.1

Parameter
=========
  ``ref`` - Root reference to be duplicated

Return
======
  Return duplicated reference or NULL if failed or ``ref`` is NULL

Tags
====
GC-Safepoint Need-Valid-Context

fh_del_ref
**********
.. code-block:: c

   void fh_del_ref(fh_object* ref)

Since
=====
Version 0.1

Parameter
=========
  ``ref`` - Root reference to be deleted

Tags
====
GC-Safepoint Need-Valid-Context

