Hello UwU
#########

Tips
****
1. Behaviour of passing NULL to any pointer argument in 
  any API call is undefined behaviour unless specified
  in the corresponding API function
2. All API functions assumed to be thread safe unless noted
3. Behaviour of creating an instance of type on a thread 
  and use on other thread is undefined behaviour unless
  specified (be it read only or writing or anything really)
4. Functions cannot initiates blocking GC unless noted
5. Uses Java's @Nullable and @Nonnull annotation because its nice

Structures
**********
  * :doc:`fluffyheap* <fluffyheap>`
  * :doc:`Mods <mods>`
  * :doc:`Context <context>`
  * :doc:`fh_object* <object>`
  * :doc:`root`
  * :doc:`fh_array* <array>`
  * :doc:`fh_descriptor* <descriptor>`


