<?xml version="1.0"?>

<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:xs="http://www.w3.org/2001/XMLSchema"
>

<xsl:output method="text"/>

<xsl:template match="xs:schema">&lt;?xml version="1.0" encoding="UTF-8"?&gt;

&lt;!-- generated using org.glite.lb.state-machine module --&gt;

&lt;jpelem:ServerConfiguration
	xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/"
	xmlns:SOAP-ENC="http://schemas.xmlsoap.org/soap/encoding/"
	xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
	xmlns:xsd="http://www.w3.org/2001/XMLSchema"
	xmlns:jptype="http://glite.org/wsdl/types/jp"
	xmlns:jpsrv="http://glite.org/wsdl/services/jp"
	xmlns:jpelem="http://glite.org/wsdl/elements/jp"&gt;

&lt;!-- List of attributes IS want to receive from PS's --&gt;
	&lt;!-- Internal attributes --&gt;
	&lt;!--
	&lt;attrs&gt;&lt;name&gt;http://egee.cesnet.cz/en/Schema/JP/System:jobId&lt;/name&gt;&lt;/attrs&gt;
	&lt;attrs&gt;&lt;name&gt;http://egee.cesnet.cz/en/Schema/JP/System:owner&lt;/name&gt;&lt;/attrs&gt;
	--&gt;
<xsl:apply-templates select="xs:element" mode="define"/>

&lt;!-- List of attributes IS will index --&gt;
	&lt;!-- default filter --&gt;
	&lt;!--&lt;indexedAttrs&gt;http://egee.cesnet.cz/en/Schema/JP/System:owner&lt;/indexedAttrs&gt;--&gt;
	&lt;!-- internal attribute (index replacement) --&gt;
	&lt;!--&lt;indexedAttrs&gt;http://egee.cesnet.cz/en/Schema/JP/System:jobId&lt;/indexedAttrs&gt;--&gt;
	&lt;indexedAttrs&gt;http://egee.cesnet.cz/en/Schema/LB/Attributes:user&lt;/indexedAttrs&gt;
	&lt;indexedAttrs&gt;http://egee.cesnet.cz/en/Schema/LB/Attributes:CE&lt;/indexedAttrs&gt;
	&lt;indexedAttrs&gt;http://egee.cesnet.cz/en/Schema/LB/Attributes:VO&lt;/indexedAttrs&gt;
	&lt;indexedAttrs&gt;http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatus&lt;/indexedAttrs&gt;

&lt;!-- List of type plugins --&gt;
	&lt;plugins&gt;&lt;/plugins&gt;

&lt;!-- List of feeds IS wants to receive from PS's--&gt;
	&lt;!-- no filter, historic batch and incremental changes --&gt;
        &lt;feeds&gt;
		&lt;!-- replace this by Job Provenance Primare Storage endpoint --&gt;
                &lt;primaryServer&gt;https://localhost:8901&lt;/primaryServer&gt;
                &lt;!-- List of conditions triggering attrs sending --&gt;
                &lt;condition&gt;
                        &lt;attr&gt;http://egee.cesnet.cz/en/Schema/JP/System:regtime&lt;/attr&gt;
                        &lt;op&gt;GREATER&lt;/op&gt;
                        &lt;value&gt;
                                &lt;string&gt;0&lt;/string&gt;
                        &lt;/value&gt;
                &lt;/condition&gt;
                &lt;history&gt;1&lt;/history&gt;
                &lt;continuous&gt;1&lt;/continuous&gt;
        &lt;/feeds&gt;
&lt;/jpelem:ServerConfiguration&gt;
</xsl:template>

<xsl:template match="xs:element" mode="define">	&lt;!-- <xsl:value-of select="xs:documentation/text()"/> --&gt;
	&lt;attrs&gt;&lt;name&gt;http://egee.cesnet.cz/en/Schema/LB/Attributes:<xsl:value-of select="@name"/>&lt;/name&gt;&lt;/attrs&gt;
</xsl:template>

</xsl:stylesheet>
