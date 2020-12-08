LIBDIR =../src
AM_CPPFLAGS = @XML_CPPFLAGS@ -I$(top_srcdir)/include
AM_CFLAGS = -I$(srcdir)/$(LIBDIR) -DSRCDIR=\"$(top_srcdir)\"
LDADD = $(LIBDIR)/libdatovka.la -L$(LIBDIR) @XML_LIBS@

noinst_PROGRAMS = isdsclient login certauth version progressbar logger \
	getuserboxinfo getuserboxinfo2 addbox changebox changebox2 \
	changepassword activate resetpassword deleteuser deleteuser2 \
	changeuser switchcommercial switchaccessibility \
	disableaccesibilityexternaly sendmultiple getsent getsignedsent \
	getsignedsentbyid getchangedstates loadreceivedmessage \
	loadcmssignedsentmessage sendxmldoc loadsentmessagewithxmldocuments \
	getreceived getsignedreceived \
	getreceivedenvelope manageuser manageuser2 markasread markasreceived \
	getsenderbyid downloadmessagehash verifyreceivedhash \
	computemessagehash authenticatemessage \
	loaddelivery getreceiveddelivery getreceivedsigneddelivery \
	convertdocument requestnewtestingbox getboxlist

common = common.c common.h

isdsclient_SOURCES = isdsclient.c $(common)
login_SOURCES = login.c $(common)
certauth_SOURCES = certauth.c $(common)
version_SOURCES = version.c $(common)
progressbar_SOURCES = progressbar.c $(common)
logger_SOURCES = logger.c $(common)
getuserboxinfo_SOURCES = getuserboxinfo.c $(common)
getuserboxinfo2_SOURCES = getuserboxinfo2.c $(common)
addbox_SOURCES = addbox.c $(common)
changebox_SOURCES = changebox.c $(common)
changebox2_SOURCES = changebox2.c $(common)
changepassword_SOURCES = changepassword.c $(common)
activate_SOURCES = activate.c $(common)
resetpassword_SOURCES = resetpassword.c $(common)
deleteuser_SOURCES = deleteuser.c $(common)
deleteuser2_SOURCES = deleteuser2.c $(common)
changeuser_SOURCES = changeuser.c $(common)
switchcommercial_SOURCES = switchcommercial.c $(common)
switchaccessibility_SOURCES = switchaccessibility.c $(common)
disableaccesibilityexternaly_SOURCES = disableaccesibilityexternaly.c $(common)
sendmultiple_SOURCES = sendmultiple.c $(common)
getsent_SOURCES = getsent.c $(common)
getsignedsent_SOURCES = getsignedsent.c $(common)
getsignedsentbyid_SOURCES = getsignedsentbyid.c $(common)
getchangedstates_SOURCES = getchangedstates.c $(common)
loadreceivedmessage_SOURCES = loadreceivedmessage.c $(common)
loadcmssignedsentmessage_SOURCES = loadcmssignedsentmessage.c $(common)
sendxmldoc_SOURCES = sendxmldoc.c $(common)
loadsentmessagewithxmldocuments_SOURCES = \
	loadsentmessagewithxmldocuments.c $(common)
getreceived_SOURCES = getreceived.c $(common)
getsignedreceived_SOURCES = getsignedreceived.c $(common)
getreceivedenvelope_SOURCES = getreceivedenvelope.c $(common)
downloadmessagehash_SOURCES = downloadmessagehash.c $(common)
verifyreceivedhash_SOURCES = verifyreceivedhash.c $(common)
computemessagehash_SOURCES = computemessagehash.c $(common)
authenticatemessage_SOURCES = authenticatemessage.c $(common)
manageuser_SOURCES = manageuser.c $(common)
manageuser2_SOURCES = manageuser2.c $(common)
markasread_SOURCES = markasread.c $(common)
markasreceived_SOURCES = markasreceived.c $(common)
getsenderbyid_SOURCES = getsenderbyid.c $(common)
loaddelivery_SOURCES = loaddelivery.c $(common)
getreceiveddelivery_SOURCES = getreceiveddelivery.c $(common)
getreceivedsigneddelivery_SOURCES = getreceivedsigneddelivery.c $(common)
convertdocument_SOURCES = convertdocument.c $(common)
requestnewtestingbox_SOURCES = requestnewtestingbox.c $(common)
getboxlist_SOURCES = getboxlist.c $(common)
