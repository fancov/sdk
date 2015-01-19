C_FILE += $(SROUCE)/util/hash/hash.c \
            $(SROUCE)/util/hash/hash_list.c

C_OBJ_FILE += hash.$(SUFFIX) \
                hash_list.$(SUFFIX)

hash.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/hash/hash.c
	
hash_list.$(SUFFIX) :
	$(C_COMPILE) $(SROUCE)/util/hash/hash_list.c


