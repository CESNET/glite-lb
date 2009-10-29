References:
-----------
[AGU] AGU Execution Service strawman rendering (draft v9), http://forge.ogf.org/sf/go/doc15628 
[GLUE] GLUE Specification v. 2.0, http://www.ogf.org/documents/GFD.147.pdf
[GLUE-EGEE09] GLUE 2.0: Rollout strategy and current status, presentation at EGEE'09 conference, 
http://indico.cern.ch/materialDisplay.py?contribId=279&sessionId=56&materialId=slides&confId=55893
[GLUE-HTML] GLUE 2.0 HTML rendering, http://cern.ch/glue20
[GLUE-REAL] GLUE 2.0 – Reference Realizations to Concrete Data Models (draft v1), http://forge.ogf.org/sf/go/doc15221
[GLUE-USECASES] GLUE 2.0 Use Cases (draft v3), http://forge.ogf.org/sf/go/doc14621
[GLUE-WG] The GLUE Working Group of OGF, http://forge.ogf.org/sf/projects/glue-wg
[GLUE-XMLSchema-xsd] GLUE 2.0 XML Schema (Draft 41, Public Comment), http://schemas.ogf.org/glue/2008/05/spec_2.0_d42_r01,
http://glueman.svn.sourceforge.net/viewvc/glueman/trunk/glue-xsd/xsd/
[GLUE-XMLSchema-html] GLUE 2.0 XML Schema (Draft 41, Public Comment) HTML rendering, http://www.desy.de/~paul/GLUE/GLUE2.html
[GLUE-XMLSchema-wiki] http://forge.ogf.org/sf/wiki/do/viewPage/projects.glue-wg/wiki/GLUE2XMLSchema
[LB-WS] Logging and Bookkeeping Web Services, http://egee.cesnet.cz/en/WSDL/

Aim:
----
implement GetActivityStatus, GetActivityInfo and QueryActivityInfo [AGU] operations
in LB (namely enhance existing LB-WS interface):

- GetActivityStatus - return states of given jobs (list of jobIds)
- GetActivityInfo - return complete state information (history?) of given jobs (list of jobIds)
- QueryActivityInfo - query all (user?) jobs

LB-WS MUST return a GLUE 2.0 compliant response.

Terminology in GLUE 2.0:
------------------------
GLUE 2 looks a bit different to GLUE 1, but most of the concepts
are there under different names (see also [GLUE-EGEE09])
– Site -> AdminDomain
– (VO) -> UserDomain
– Element -> Service, Service -> Endpoint
– AccessControlBaseRule -> AccessPolicy, MappingPolicy
– CE, VOView -> ComputingManager, ComputingShare
– Cluster/SubCluster -> ExecutionEnvironment
– (Job) -> Activity
– SA/VOInfo -> StorageShare

Open issues:
------------
- mapping of LB states to AGU states (see [AGU], section 5)

Off topic:
----------
- could the following be applicable to LB?
-- SQL Schema Realization of GLUE 2.0 (see [GLUE-REAL], section 4)
-- LDAP Schema Realization (see [GLUE-REAL], section 5)



