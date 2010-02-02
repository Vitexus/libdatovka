LIBDIR =../src
AM_CFLAGS =-std=c99 -Wall -Werror -g
# Large files needed by GPGME
AM_CFLAGS += -D_FILE_OFFSET_BITS=64 -DLARGEFILE_SOURCE=1
AM_CFLAGS +=-I$(LIBDIR)
LDADD = $(LIBDIR)/libisds.la -L$(LIBDIR)

noinst_PROGRAMS = isdsclient login progressbar getuserboxinfo changebox \
	changepassword resetpassword deleteuser changeuser \
	switchcommercial switchaccessibility disableaccesibilityexternaly \
	sendmultiple getsent  getsignedsent \
	loadreceivedmessage loadcmssignedsentmessage \
	getreceived getsignedreceived \
	getreceivedenvelope markasread markasreceived \
	downloadmessagehash verifyreceivedhash computemessagehash \
	loaddelivery getreceiveddelivery getreceivedsigneddelivery

common = common.c common.h

isdsclient_SOURCES = isdsclient.c $(common)
login_SOURCES = login.c $(common)
progressbar_SOURCES = progressbar.c $(common)
getuserboxinfo_SOURCES = getuserboxinfo.c $(common)
changebox_SOURCES = changebox.c $(common)
changepassword_SOURCES = changepassword.c $(common)
resetpassword_SOURCES = resetpassword.c $(common)
deleteuser_SOURCES = deleteuser.c $(common)
changeuser_SOURCES = changeuser.c $(common)
switchcommercial_SOURCES = switchcommercial.c $(common)
switchaccessibility_SOURCES = switchaccessibility.c $(common)
disableaccesibilityexternaly_SOURCES = disableaccesibilityexternaly.c $(common)
sendmultiple_SOURCES = sendmultiple.c $(common)
getsent_SOURCES = getsent.c $(common)
getsignedsent_SOURCES = getsignedsent.c $(common)
loadreceivedmessage_SOURCES = loadreceivedmessage.c $(common)
loadcmssignedsentmessage_SOURCES = loadcmssignedsentmessage.c $(common)
getreceived_SOURCES = getreceived.c $(common)
getsignedreceived_SOURCES = getsignedreceived.c $(common)
getreceivedenvelope_SOURCES = getreceivedenvelope.c $(common)
downloadmessagehash_SOURCES = downloadmessagehash.c $(common)
verifyreceivedhash_SOURCES = verifyreceivedhash.c $(common)
computemessagehash_SOURCES = computemessagehash.c $(common)
markasread_SOURCES = markasread.c $(common)
markasreceived_SOURCES = markasreceived.c $(common)
loaddelivery_SOURCES = loaddelivery.c $(common)
getreceiveddelivery_SOURCES = getreceiveddelivery.c $(common)
getreceivedsigneddelivery_SOURCES = getreceivedsigneddelivery.c $(common)

