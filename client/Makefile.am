LIBDIR =../src
AM_CPPFLAGS = @XML_CPPFLAGS@ -I$(top_srcdir)/include
AM_CFLAGS = -I$(srcdir)/$(LIBDIR) -DSRCDIR=\"$(top_srcdir)\"
LDADD = $(LIBDIR)/libdatovka.la -L$(LIBDIR) @XML_LIBS@

noinst_PROGRAMS = \
	activate addbox authenticatemessage certauth changebox changebox2 \
	changepassword changeuser check_time_conversion computemessagehash \
	convertdocument createbigmessage deleteuser deleteuser2 \
	disableaccesibilityexternaly \
	downloadmessagehash dtinfo getboxlist getchangedstates getreceived \
	getreceiveddelivery getreceivedenvelope getreceivedsigneddelivery \
	getsenderbyid getsent getsignedreceived getsignedsent \
	getsignedsentbyid getuserboxinfo getuserboxinfo2 isdsclient \
	loadcmssignedsentmessage loaddelivery loadreceivedmessage \
	loadsentmessagewithxmldocuments logger login manageuser manageuser2 \
	markasread markasreceived pdz_info progressbar requestnewtestingbox \
	resetpassword sendmultiple sendxmldoc signedsentbigmessagedownload \
	switchaccessibility \
	switchcommercial uploadattachment verifyreceivedhash version

common = common.c common.h

activate_SOURCES = activate.c $(common)
addbox_SOURCES = addbox.c $(common)
authenticatemessage_SOURCES = authenticatemessage.c $(common)
certauth_SOURCES = certauth.c $(common)
changebox_SOURCES = changebox.c $(common)
changebox2_SOURCES = changebox2.c $(common)
changepassword_SOURCES = changepassword.c $(common)
changeuser_SOURCES = changeuser.c $(common)
check_time_conversion_SOURCES = check_time_conversion.c $(common)
computemessagehash_SOURCES = computemessagehash.c $(common)
convertdocument_SOURCES = convertdocument.c $(common)
createbigmessage_SOURCES = createbigmessage.c $(common)
deleteuser_SOURCES = deleteuser.c $(common)
deleteuser2_SOURCES = deleteuser2.c $(common)
disableaccesibilityexternaly_SOURCES = disableaccesibilityexternaly.c $(common)
downloadmessagehash_SOURCES = downloadmessagehash.c $(common)
dtinfo_SOURCES = dtinfo.c $(common)
getboxlist_SOURCES = getboxlist.c $(common)
getchangedstates_SOURCES = getchangedstates.c $(common)
getreceived_SOURCES = getreceived.c $(common)
getreceiveddelivery_SOURCES = getreceiveddelivery.c $(common)
getreceivedenvelope_SOURCES = getreceivedenvelope.c $(common)
getreceivedsigneddelivery_SOURCES = getreceivedsigneddelivery.c $(common)
getsenderbyid_SOURCES = getsenderbyid.c $(common)
getsent_SOURCES = getsent.c $(common)
getsignedreceived_SOURCES = getsignedreceived.c $(common)
getsignedsent_SOURCES = getsignedsent.c $(common)
getsignedsentbyid_SOURCES = getsignedsentbyid.c $(common)
getuserboxinfo_SOURCES = getuserboxinfo.c $(common)
getuserboxinfo2_SOURCES = getuserboxinfo2.c $(common)
isdsclient_SOURCES = isdsclient.c $(common)
loadcmssignedsentmessage_SOURCES = loadcmssignedsentmessage.c $(common)
loaddelivery_SOURCES = loaddelivery.c $(common)
loadreceivedmessage_SOURCES = loadreceivedmessage.c $(common)
loadsentmessagewithxmldocuments_SOURCES = loadsentmessagewithxmldocuments.c $(common)
logger_SOURCES = logger.c $(common)
login_SOURCES = login.c $(common)
manageuser_SOURCES = manageuser.c $(common)
manageuser2_SOURCES = manageuser2.c $(common)
markasread_SOURCES = markasread.c $(common)
markasreceived_SOURCES = markasreceived.c $(common)
pdz_info_SOURCES = pdz_info.c $(common)
progressbar_SOURCES = progressbar.c $(common)
requestnewtestingbox_SOURCES = requestnewtestingbox.c $(common)
resetpassword_SOURCES = resetpassword.c $(common)
sendmultiple_SOURCES = sendmultiple.c $(common)
sendxmldoc_SOURCES = sendxmldoc.c $(common)
signedsentbigmessagedownload_SOURCES = signedsentbigmessagedownload.c $(common)
switchaccessibility_SOURCES = switchaccessibility.c $(common)
switchcommercial_SOURCES = switchcommercial.c $(common)
uploadattachment_SOURCES = uploadattachment.c $(common)
verifyreceivedhash_SOURCES = verifyreceivedhash.c $(common)
version_SOURCES = version.c $(common)
