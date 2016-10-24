#
# Operating System and Developing system specific macros
#

# target library file full name
TRGTLIB = libosipparser2.a

#
# Target dependent macros
#

MODULES = osip_accept osip_accept_encoding osip_accept_language osip_alert_info osip_allow \
	    osip_authentication_info osip_authorization osip_body osip_call_id osip_call_info osip_contact \
		osip_content_disposition osip_content_encoding osip_content_length \
		osip_content_type osip_cseq osip_error_info osip_from osip_header \
		osip_list osip_md5c osip_message osip_message_parse osip_message_to_str \
		osip_mime_version osip_parser_cfg osip_proxy_authenticate \
		osip_proxy_authentication_info osip_proxy_authorization \
		osip_record_route osip_route osip_to osip_uri osip_via osip_www_authenticate \
		sdp_accessor sdp_message sdp_negoc stun_parser

BUILD_RELEASE = 2
include ../makefile.inc

$(TRGTLIB) : $(OBJECTS)
	$(AR) -r $(TRGTLIB) $(OBJECTS)

clean:
	$(RM) -f $(OBJECTS)
	$(RM) -f $(TRGTLIB)
