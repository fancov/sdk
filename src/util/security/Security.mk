CFILE += $(SROUCE)/util/security/crypto_lib.c \
	$(SROUCE)/util/security/digest_lib.c \
	$(SROUCE)/util/security/crc.c \
	
C_OBJ_FILE += crypto_lib.$(SUFFIX) \
	digest_lib.$(SUFFIX) \
	crc.$(SUFFIX)

crypto_lib.$(SUFFIX):
	$(C_COMPILE) $(SROUCE)/util/security/crypto_lib.c
digest_lib.$(SUFFIX):
	$(C_COMPILE) $(SROUCE)/util/security/digest_lib.c

crc.$(SUFFIX):
	$(C_COMPILE) $(SROUCE)/util/security/crc.c