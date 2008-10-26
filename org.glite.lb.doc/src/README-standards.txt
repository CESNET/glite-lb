Logging and Bookkeeping (LB) service and standards
==================================================

The aim of this document is to list all possible IT standards related to the 
Logging and Bookkeeping (LB) service togehter with the recommendations what should
be changed in the LB code to support the standards as well as with the proposals 
to change also the standard specifications. There is no particular order in the list.

Good starting points for reading are:

[OGSA-Standards] Defining the Grid: A Roadmap for OGSA® Standards
OGF Informational Document GFD-I.123 http://www.ogf.org/documents/GFD.123.pdf
- overview of the many interrelated recommendations and
informational documents being produced by the OGSA

[OGSA-Architecture] The Open Grid Services Architecture, Version 1.5
OGF Informational Document GFD-I.080 http://www.ogf.org/documents/GFD.80.pdf
- Job Tracking not explicitly mentioned here, maybe we could propose OGSA-LB? :)

Standards and Web services
- see http://www.ibm.com/developerworks/webservices/standards/


Command Line + User Interface Standards
---------------------------------------

- support for Internationalization (i18n) and localization (l10n), 
in particular UTF-8 support in user tags
-- switch all output to LC_MESSAGES compliant format

- standard error codes from all CLI tools
-- define on gLite level


Coding and Language Standards
-----------------------------

[WSDL] Web Services Description Language (WSDL) 1.1
W3C Note http://www.w3.org/TR/wsdl
- check if LB.wsdl and LBTypes.wsdl satisfy this specification

[XML] Extensible Markup Language (XML) 1.1 (Second Edition)
W3C Recommendation http://www.w3.org/TR/xml11
- check if all XMLs in LB satisfy this specification 

[XML-Namespaces] Namespaces in XML 1.1 (Second Edition)
W3C Recommendation http://www.w3.org/TR/xml-names11
- check if all XML namespaces in LB satisfy this specification 

[SOAP] SOAP Version 1.2
W3C Recommendation http://www.w3.org/TR/soap12
- check SOAP in security.gsoap-plugin 
- see also W3C Working Group XML Protocol: http://www.w3.org/2000/xp/Group/

[GNU] GNU Coding Standards: http://www.gnu.org/prep/standards/
- DISCUSSION: adopt these coding standards to the code of LB?


Network Protocols Standards
---------------------------

[IPv6] Internet Protocol version 6 (IPv6) 
IETF Standard, for IPv6 related specifications see http://www.ipv6.org/specs.html
- all communication in LB must be modified to fully support IPv6
- affected modules: [org.glite.]jobid, security.gss, security.gsoap-plugin, lb.common, 
lb.client, lb.logger, lb.server

[SNMP] Simple Network Management Protocol (SNMP), Version 3 
IETF Standard, for list of related RFCs see http://en.wikipedia.org/wiki/SNMP#RFCs
- SNMPv3: http://www.ibr.cs.tu-bs.de/projects/snmpv3/
- create private enterprise numbers for gLite under the prefix: 
iso.org.dod.internet.private.enterprise (1.3.6.1.4.1)
- create MIB for LB and implement basic monitoring functions (lb.server)


Job related standards
---------------------

[JSDL] Job Submission Description Language (JSDL) Specification, Version 1.0 
OGF Recommendation GFD-R.136 http://www.ogf.org/documents/GFD.136.pdf
- specifies the semantics and structure of JSDL and includes the normative XML Schema for JSDL
- LB should be aware of the JSDL format since some of the attributes are (will be) read from it

[JSDL-exp] Implementation and Interoperability Experiences with the Job Submission Description Language(JSDL) 1.0 
OGF Experimental Document GFD-E.140 http://www.ogf.org/documents/GFD.140.pdf
- good summary of different JSDL implementations and interoperability experiences
- LB should be aware of different JSDL implementations 

[OGSA-BES] OGSA Basic Execution Service, Version 1.0 
OGF Proposed Recommendation GFD-R-P.108 http://www.ogf.org/documents/GFD.108.pdf
- defines a basic state model with states:
pending, running, finished, terminated, failed (last three terminal states)
- plus specializations: 
running -> running stage-in, running executing, running stage-out
running -> running proceeded, running suspended (with events resume, suspend)
running -> running on-resource, running migrating (with event migrate)
running -> running can-proceed, running held (with events hold, release)
- LB could provide information for BES-Factory and BES-Activity port-type 
BES-Factory port-type defines operations for initiating, monitoring, and managing 
sets of activities, and for accessing information about the BES. 
BES-Activity port-type indicates an extension point for operations relating to the 
monitoring and management of individual activities. BES-Activity is unspecified
and maybe it could be extended by LB specification in the future.
- reflect the above states specializations in the LB state machine


WS Standards
------------

[WS-I-BP] The Web Services-Interoperability Organization (WS-I) Basic Profile
Version 1.1 (Specification) http://www.ws-i.org/Profiles/BasicProfile-1.1.html
Version 1.2 (Board Approval Draft) http://www.ws-i.org/Profiles/BasicProfile-1.2.html
Version 2.0 (Working Group Draft) http://www.ws-i.org/Profiles/BasicProfile-2_0(WGD).html
- defines a set of non-proprietary WS specifications, along with clarifications, 
refinements, interpretations and amplifications of those specifications which promote 
interoperability
- rather general specification of WS interoperability, defines messages and service description
(esp. SOAP Bindings)
- check if LB WS needs to change

[WS-Addresing] Web Services Addressing 1.0 - Core
W3C Recommendation http://www.w3.org/TR/ws-addr-core
+
[WS-Naming] WS-Naming Specification
OGF Proposed Recommendation GFD-R-P.109 http://www.ogf.org/documents/GFD.109.pdf
- in LB define properly EndPoint Identifier (EPI)


WS Data Acces and Integration
-----------------------------

[WS-DAI] Web Services Data Access and Integration - The Core (WS-DAI) Specification, Version 1.0 
OGF Proposed Recommendation GFD-R-P.074 http://www.ogf.org/documents/GFD.74.pdf
- specification for a collection of generic data interfaces that are made available as WS
- describe basic concepts (data service model, interface, data access, lifetime) 
and data service properties and messages (static and configurable data description, data access and 
factory messages)
- this is general specification, see particular proposals to access relational [WS-DAIR] and 
XML [WS-DAIX] representations of data below
- there are probably no particular needs to change this specification (at the moment)
- LB probably COULD be WS-DAI compliant and implement at least one of the WS-DAIR or WS-DAIX 
realisations - do we really want it?


[WS-DAIX] Web Services Data Access and Integration - The XML Realization (WS-DAIX) Specification, Version 1.0 
OGF Proposed Recommendation GFD-R-P.075 http://www.ogf.org/documents/GFD.75.pdf
- specification for a collection of data access interfaces for XML data resources
- it is NOT a new query/update language for XML, it works as a "channel" to XPath/XQuery/XUpdate
- defines new interfaces XMLCollectionDescription and XMLCollectionAccess
-- direct access to data: XPathAccess/XQueryAccess/XUpdateAccess across a XMLCollection
-- indirect access: XPathFactory/XQueryFactory/XUpdateFactory across a XMLCollection
- XMLCollection collects together XML documents (MAY be nested)
-- several different approaches in LB possible: "single document" could be a 
job state description -> collection of jobs
one event -> collection of events 
one query -> complex queries
-- XMLCollectionAccess then defines operations 
Add/Get/Remove Documents, Create/Remove Subcollection, Add/Get/Remove Schema
- XMLSequence represents the result sequence of an XQuery or XPath request. A sequence is
an ordered collection of zero or more items (node, atomic value)
- implementation in LB MAY be either easy (update already used XML to WS-DAIX "format")
or very difficult (keep data on LB server in one or more different XML formats)


[WS-DAIR] Web Services Data Access and Integration - The Relational Realisation (WS-DAIR) Specification, Version 1.0 
OGF Proposed Recommendation GFD-R-P.076 http://www.ogf.org/documents/GFD.76.pdf
- specification for a collection of data access interfaces for relational data resources
- defines a new direct access interfaces SQLAccessDescription and SQLAccess
and indirect SQLAccessFactory and SQLResponseFactory
- SQLAccess then defines operations 
GetSQLPropertyDocument (returns SQLAccessDescription, i.e. static schema + dataset description) and 
SQLExecute (IN: SQLExecuteRequest, OUT: SQLExecuteResponse)
- response is defined by SQLResponseDescription/Access, 
-- it can contain SQLRowSets - results of an SQL query over the database, it is defined by SQLRowSetDescription/Access 
- implementation in LB could be easier than for WS-DAIX
- could be an independent service on top of the LB database (or directly in the lb.server?)
- should not be difficult to define the AccessDescription and implement Access


[DAIS-InterTest] Interoperability Testing for DAIS Working Group Specifications 
OGF Informational Document GFD-I.077 http://www.ogf.org/documents/GFD.77.pdf
- describes how to test WS-DAI* implementations (mandatory and optional features)
- once LB implements WS-DAI*, these tests will have to be performed


WS Notifications
----------------

WS-Notification is a family of related specifications that define a standard Web services 
approach to notification using a topic-based publish/subscribe pattern.
- see http://www.oasis-open.org/committees/tc_home.php?wg_abbrev=wsn

[WS-BaseNotification] Web Services Base Notification 1.3
OASIS Standard http://docs.oasis-open.org/wsn/wsn-ws_base_notification-1.3-spec-os.pdf
- standard message exchanges to be implemented by service providers that wish to participate in Notifications
- defines notification consumer and producer interface, pull-style notifications,
subscription manager interface, and some security considerations
- Notifications via WS is a new functionality to LB that should not be too difficult to implement

[WS-BrokeredNotification] Web Services Brokered Notification 1.3
OASIS Standard http://docs.oasis-open.org/wsn/wsn-ws_brokered_notification-1.3-spec-os.pdf
- standard message exchanges for a notification broker service provider (allowing publication 
of messages from entities that are not themselves service providers)
- a notification broker is an intermediary between message publishers and message subscribers
- possible implementation in LB might consider a notification broker rather than implementing
the functionality directly within the LB server

[WS-Topics] Web Services Topics 1.3
OASIS Standard http://docs.oasis-open.org/wsn/wsn-ws_topics-1.3-spec-os.pdf
- defines a mechanism to organize and categorize items of interest for subscription known as “topics”
- defines four topic expression dialects that can be used as subscription expressions in subscribe 
request messages and other parts of the WS-Notification system
- specifies an XML model for describing metadata associated with topics
- this document should be considered during implementation of WS-Notifications


WS Reliable Messaging
---------------------

[WS-RM] Web Services ReliableMessaging 1.1 
OASIS Standard http://docs.oasis-open.org/ws-rx/wsrm/200702/wsrm-1.1-spec-os-01-e1.html
- describes a protocol that allows messages to be delivered reliably between 
distributed applications in the presence of software component, system, or network failures
- see also https://www.ibm.com/developerworks/webservices/library/specification/ws-rm/
- it probably SHOULD be considered when implementing LB WS-Notifications as well as 
in the standard WS messaging provided by LB


WS Security
-----------

[WS-I-BSP] The Web Services-Interoperability Organization (WS-I) Basic Security Profile
Version 1.0 (Specification) http://www.ws-i.org/Profiles/BasicSecurityProfile-1.0.html
Version 1.1 (Working Group Approval Draft) http://www.ws-i.org/Profiles/BasicSecurityProfile-1.1.html
- defines transport layer mechanisms, SOAP nodes and messages, security headers, timestamps, 
security tokens (X.509, Kerberos, SAML, EncryptedKey), XML Signature and XML Encryption,
attachment security
- LB should probably adopt all these mechanisms

[WS-I-RSP] The Web Services-Interoperability Organization (WS-I) Reliable Security Profile
Version 1.0 (Working Group Draft) http://www.ws-i.org/Profiles/ReliableSecureProfile-1.0.html
- defines secure reliable messaging and secure conversation
- LB should probably adopt all these mechanisms
- see also Reliable Secure Profile Usage Scenarios http://www.ws-i.org/profiles/rsp-scenarios-1.0.pdf

[Sec-addr] Secure Addressing Profile 1.0
OGF Proposed Recommendation GFD-R-P.131 http://www.ogf.org/documents/GFD.131.pdf
- defines WS-SecurityPolicy assertions within WS-Addressing
- in LB define properly EndPoint Reference (EPR)
- affects all LB WS (ljocha: especially LB notifications?)

[Sec-comm] Secure Communication Profile 1.0 
OGF Proposed Recommendation GFD-R-P.132 http://www.ogf.org/documents/GFD.132.pdf
- defines authentication, authorization (and auditing), integrity and confidentiality properties of WS interactions
- affects all LB WS, LB SHOULD adopt:
-- Message-level authentication (see section 7)
-- Integrity (e.g. CRITICAL_SIGNING of SOAP messages)
-- more transport-level mechanisms and policies (see section 6)
- proper LB security audit required


WS Transactions
---------------

WS Transactions specifications define mechanisms for transactional interoperability between 
WS domains and provide a means to compose transactional qualities of service into WS applications.
- see http://www.ibm.com/developerworks/webservices/library/specification/ws-tx
- describes an extensible coordination framework (WS-Coordination) and specific coordination types for
short duration, ACID transactions (WS-AtomicTransaction) and longer running business transactions (WS-BusinessActivity)
- LB should consider WS-AtomicTransaction when logging via WS is going to be supported
(org.glite.lb.java-client?) and WS-BusinessActivity probably for WS queries


Other references
----------------

[OASIS] OASIS Standards and Other Approved Work
- see http://www.oasis-open.org/specs/

[OGF] OGF Documents
- http://www.ogf.org/gf/docs/?final

[W3C] W3C Technical Reports and Publications
- see http://www.w3.org/TR/

[STD] IETF Official Internet Protocol Standards 
- see http://www.rfc-editor.org/rfcxx00.html

[IETF] IETF Internet-Drafts
- see http://www.ietf.org/ID.html

