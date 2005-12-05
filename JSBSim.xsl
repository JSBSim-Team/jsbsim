<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
    <xsl:template match="/fdm_config">
        <html>
            <head>
                <title><xsl:value-of select="@name"/></title>
            </head>
            <body style="font-family=Arial;font-size=90%">
                <a name="top"/>
                <font face="Arial" size="3" color="224488"><b><xsl:value-of select="@name"/></b></font><br/>
                <font face="Arial" size="2">Configuration File Version: <xsl:value-of select="@version"/></font><br/>
                <hr width="100%"/><font face="Arial" size="1">
                [<a href="#fileheader">File Information</a>] [<a href="#metrics">Metrics</a>] [<a href="#massbalance">Mass and Balance</a>] 
                [<a href="#groundreactions">Ground Reactions</a>] [<a href="#propulsion">Propulsion</a>] [<a href="#autopilot">Autopilot</a>] 
                [<a href="#flightcontrol">Flight Control</a>] [<a href="#aerodynamics">Aerodynamics</a>] [<a href="#input">Input</a>] 
                [<a href="#output">Output</a>]</font><p/>
                <table width="100%">

                    <!-- FILEHEADER -->
                    <a name="fileheader"/>
                    <tr bgcolor="EEEEEE">
                        <table width="100%" bgcolor="EEEEEE" cellpadding="0" cellspacing="0" style="font-family=Arial; font-size=90%">
                                <tr><td colspan="4"><b>FILE INFORMATION</b></td></tr>
                            <tr><td colspan="4" height="3"><hr size="1"/></td></tr>
                            <tr><td><b>Author[s]</b></td><td valign="top" colspan="3" align="left">
                            <xsl:for-each select="fileheader/author" >
                                <xsl:value-of select="."/>, 
                            </xsl:for-each>
                            </td></tr>
                                <tr><td><b>File created</b></td><td colspan="3" align="left"><xsl:value-of select="fileheader/filecreationdate"/></td></tr> 
                                <tr><td><b>Description</b></td><td colspan="3" align="left"><xsl:value-of select="fileheader/description"/></td></tr>
                                <tr><td><b>Model version information</b></td><td colspan="3"><xsl:value-of select="fileheader/version"/></td></tr>
                                <tr><td><b>References:</b><br/></td><td></td><td></td><td></td></tr>
                                <xsl:for-each select="fileheader/reference">
                                <tr>
                                <td><xsl:value-of select="@refID"/></td>
                                <td><i><xsl:value-of select="@title"/></i></td>
                                <td><xsl:value-of select="@author"/></td>
                                <td><xsl:value-of select="@date"/></td>
                                </tr>
                            </xsl:for-each>
                            </table>
                    </tr>

<tr><br/><font face="Arial" size="2">[<a href="#top">Top</a>]</font><p/></tr>

                    <!-- METRICS -->
                    <a name="metrics"/>
                    <tr bgcolor="DDEEFF">
                        <table width="100%" style="font-family=Arial; font-size=90%" bgcolor="EEEEEE" cellpadding="0" cellspacing="0">
                            <tr><td colspan="2"><b>METRICS</b></td></tr>
                            <tr><td colspan="2" height="3"><hr size="1"/></td></tr>
                            <!-- wingspan -->
                            <tr><td width="300"><b>Wing span <xsl:if test="metrics/wingspan/@unit != 0">(<xsl:value-of select="metrics/wingspan/@unit"/>)</xsl:if></b></td>
                                <td align="left"><xsl:value-of select="metrics/wingspan"/></td></tr>
                            <!-- Wing area -->
                            <tr><td width="300"><b>Wing area <xsl:if test="metrics/wingarea/@unit != 0">(<xsl:value-of select="metrics/wingarea/@unit"/>)</xsl:if></b></td>
                                <td><xsl:value-of select="metrics/wingarea"/></td></tr>
                            <!-- Chord -->
                            <tr><td width="300"><b>Chord <xsl:if test="metrics/chord/@unit != 0">(<xsl:value-of select="metrics/chord/@unit"/>)</xsl:if></b></td>
                                <td><xsl:value-of select="metrics/chord"/></td></tr>
                            <!-- horizontal tail area -->
                            <xsl:if test="metrics/htailarea">
                                <tr><td width="300"><b>Horizontal tail area <xsl:if test="metrics/htailarea/@unit != 0">(<xsl:value-of select="metrics/htailarea/@unit"/>)</xsl:if></b></td>
                                <td><xsl:value-of select="metrics/htailarea"/></td></tr>
                              </xsl:if>
                            <!-- Horizontal tail arm -->
                            <xsl:if test="metrics/htailarm">
                                <tr><td width="300"><b>Horizontal tail arm <xsl:if test="metrics/htailarm/@unit != 0">(<xsl:value-of select="metrics/htailarm/@unit"/>)</xsl:if></b></td>
                                    <td><xsl:value-of select="metrics/htailarm"/></td></tr>
                            </xsl:if>
                            <!-- Horizontal tail area -->
                            <xsl:if test="metrics/htailarea">
                                <tr><td width="300"><b>Vertical tail area <xsl:if test="metrics/vtailarea/@unit != 0">(<xsl:value-of select="metrics/vtailarea/@unit"/>)</xsl:if></b></td>
                                <td><xsl:value-of select="metrics/vtailarea"/></td></tr>
                            </xsl:if>
                            <!-- Vertical tail arm -->
                            <xsl:if test="metrics/vtailarm">
                                <tr><td width="300"><b>Vertical tail arm <xsl:if test="metrics/vtailarm/@unit != 0">(<xsl:value-of select="metrics/vtailarm/@unit"/>)</xsl:if></b></td>
                                    <td><xsl:value-of select="metrics/vtailarm"/></td></tr>
                            </xsl:if>
                            <!-- locations -->
                            <xsl:for-each select="metrics/location">
                                <xsl:choose>
                                    <xsl:when test="@name = 'AERORP'">
                                        <tr><td width="300"><b>Aerodynamic reference point location (<xsl:value-of select="@unit"/>) </b></td>
                                          <td><xsl:value-of select="x"/>, <xsl:value-of select="y"/>, <xsl:value-of select="z"/></td></tr>
                                    </xsl:when>
                                    <xsl:when test="@name = 'VRP'">
                                        <tr><td width="300"><b>Visual reference point location (<xsl:value-of select="@unit"/>): </b></td>
                                            <td><xsl:value-of select="x"/>, <xsl:value-of select="y"/>, <xsl:value-of select="z"/></td></tr>
                                    </xsl:when>
                                    <xsl:when test="@name = 'EYEPOINT'">
                                        <tr><td width="300"><b>Eyepoint location (<xsl:value-of select="@unit"/>): </b></td>
                                            <td><xsl:value-of select="x"/>, <xsl:value-of select="y"/>, <xsl:value-of select="z"/></td></tr>
                                    </xsl:when>
                                    <xsl:otherwise><font color="FF0000"><tr><td colspan="2"><xsl:value-of select="@name"/> is an invalid location!</td></tr></font><br/></xsl:otherwise>
                                </xsl:choose>
                            </xsl:for-each>
                            </table>
                        </tr>
                    
<tr><br/><font face="Arial" size="2">[<a href="#top">Top</a>]</font><p/></tr>
                    
                    <!-- MASS and BALANCE -->
                    <a name="massbalance"/>
                    <tr bgcolor="DDEEFF">
                            <table width="100%" style="font-family=Arial; font-size=90%" bgcolor="EEEEEE" cellpadding="0" cellspacing="0">
                                <tr><td colspan="2"><b>MASS AND BALANCE</b></td></tr>
                                <tr><td colspan="2" height="3"><hr size="1"/></td></tr>
                                <tr><td colspan="2">Moments of Inertia</td></tr>
                            <xsl:if test="mass_balance/ixx">
                                <tr><td width="300"><b>Ixx <xsl:if test="mass_balance/ixx/@unit != 0">(<xsl:value-of select="mass_balance/ixx/@unit"/>)</xsl:if></b></td>
                                    <td><xsl:value-of select="mass_balance/ixx"/></td></tr>
                            </xsl:if>
                            <xsl:if test="mass_balance/iyy">
                                <tr><td width="300"><b>Iyy <xsl:if test="mass_balance/iyy/@unit != 0">(<xsl:value-of select="mass_balance/iyy/@unit"/>)</xsl:if></b></td>
                                    <td><xsl:value-of select="mass_balance/iyy"/></td></tr>
                            </xsl:if>
                            <xsl:if test="mass_balance/izz">
                                <tr><td width="300"><b>Izz <xsl:if test="mass_balance/izz/@unit != 0">(<xsl:value-of select="mass_balance/izz/@unit"/>)</xsl:if></b></td>
                                    <td><xsl:value-of select="mass_balance/izz"/></td></tr>
                            </xsl:if>
                            <xsl:if test="mass_balance/ixy">
                                <tr><td width="300"><b>Ixy <xsl:if test="mass_balance/ixy/@unit != 0">(<xsl:value-of select="mass_balance/ixy/@unit"/>)</xsl:if></b></td>
                                    <td><xsl:value-of select="mass_balance/ixy"/></td></tr>
                            </xsl:if>
                            <xsl:if test="mass_balance/ixz">
                                <tr><td width="300"><b>Ixz <xsl:if test="mass_balance/ixz/@unit != 0">(<xsl:value-of select="mass_balance/ixz/@unit"/>)</xsl:if></b></td>
                                    <td><xsl:value-of select="mass_balance/ixz"/></td></tr>
                            </xsl:if>
                            <xsl:if test="mass_balance/iyz">
                                <tr><td width="300"><b>Iyz <xsl:if test="mass_balance/iyz/@unit != 0">(<xsl:value-of select="mass_balance/iyz/@unit"/>)</xsl:if></b></td>
                                    <td><xsl:value-of select="mass_balance/iyz"/></td></tr>
                            </xsl:if>
                                </table>
                    </tr>

<tr><br/><font face="Arial" size="2">[<a href="#top">Top</a>]</font><p/></tr>

                    <!-- GROUND REACTIONS -->
                    <a name="groundreactions"/>
                    <tr bgcolor="DDEEFF">
                            <table width="100%" style="font-family=Arial; font-size=90%" bgcolor="EEEEEE" cellpadding="0" cellspacing="0">
                                <tr><td colspan="2"><b>GROUND REACTIONS</b></td></tr>
                                <tr><td colspan="2" height="3"><hr size="1"/></td></tr>
                            </table>
                    </tr>
                    
<tr><br/><font face="Arial" size="2">[<a href="#top">Top</a>]</font><p/></tr>
                    
                    <!-- PROPULSION -->
                    <a name="propulsion"/>
                    <tr bgcolor="DDEEFF">
                            <table width="100%" style="font-family=Arial; font-size=90%" bgcolor="DDEEFF" cellpadding="0" cellspacing="0">
                                <tr><td colspan="2"><b>PROPULSION</b></td></tr>
                                <tr><td colspan="2" height="3"><hr size="1"/></td></tr>
                            </table>
                    </tr>
                    
<tr><br/><font face="Arial" size="2">[<a href="#top">Top</a>]</font><p/></tr>
                    
                    <!-- AUTOPILOT -->
                    <a name="autopilot"/>
                    <tr bgcolor="FFDDDD">
                            <table width="100%" style="font-family=Arial; font-size=90%" bgcolor="FFDDDD" cellpadding="0" cellspacing="0">
                                <tr><td colspan="2"><b>AUTOPILOT</b></td></tr>
                                <tr><td colspan="2" height="3"><hr size="1"/></td></tr>
                            </table>
                    </tr>                    

<tr><br/><font face="Arial" size="2">[<a href="#top">Top</a>]</font><p/></tr>
                    
                    <!-- FLIGHT CONTROL -->
                    <a name="flightcontrol"/>
                    <tr bgcolor="FFDDDD">
                        <table width="100%" style="font-family=Arial; font-size=90%" bgcolor="FFDDDD" cellpadding="0" cellspacing="0">
                            <tr><td colspan="2"><b>FLIGHT CONTROL</b></td></tr>
                            <tr><td colspan="2" height="3"><hr size="1"/></td></tr>
                            <xsl:for-each select="flight_control/channel"><tr><td valign="top"><font face="Arial" size="2"><b>Channel </b><xsl:value-of select="@name"/></font></td>
                                    <td><xsl:for-each select="component">
                                        <font face="Arial" size="2">
                                            <p><b>Component: </b><xsl:value-of select="@name"/>, Type: 
                                            <xsl:choose>
                                                <xsl:when test="@type = 'LAG_FILTER'">
                                                    <i>Lag filter</i><br/>
                                                    <xsl:if test="input">Input: <i><xsl:value-of select="input"/></i><br/></xsl:if>
                                                    <table>
                                                        <tr><td nowrap="1" valign="center" align="left"><font face="Arial"
                                                        size="2">Filter:</font></td><td align="center" nowrap="1"><font face="Arial" size="2">
                                                        <xsl:value-of select="c1"/><br/><hr width="100%"/>
                                                        s + <xsl:value-of select="c1"/>
                                                        </font></td></tr>
                                                    </table>
                                                </xsl:when>
                                                <xsl:when test="@type =  'LEAD_LAG_FILTER'" >
                                                    <i>Lead-lag filter</i><br/>
                                                    <xsl:if test="input">Input: <i><xsl:value-of select="input"/></i><br/></xsl:if>
                                                    <table>
                                                        <tr><td nowrap="1" valign="center" align="left"><font face="Arial"
                                                        size="2">Filter:</font></td><td align="center" nowrap="1"><font face="Arial" size="2">
                                                        <xsl:if test="count(c1)>0"><xsl:value-of select="c1"/>s</xsl:if>
                                                        <xsl:if test="count(c1)=1 and count(c2)=1"> + </xsl:if>
                                                        <xsl:if test="count(c2)>0"><xsl:value-of select="c2"/></xsl:if>
                                                        <br/><hr width="100%"/>
                                                        <xsl:if test="count(c3)>0"><xsl:value-of select="c3"/>s</xsl:if>
                                                        <xsl:if test="count(c3)=1 and count(c4)=1"> + </xsl:if>
                                                        <xsl:if test="count(c4)>0"><xsl:value-of select="c4"/></xsl:if>
                                                        </font></td></tr>
                                                    </table>
                                                </xsl:when>
                                                <xsl:when test="@type =  'SECOND_ORDER_FILTER'" >
                                                    <i>Second order filter</i><br/>
                                                    <xsl:if test="input">Input: <i><xsl:value-of select="input"/></i><br/></xsl:if>
                                                    <table>
                                                        <tr><td nowrap="1" valign="center" align="left"><font face="Arial"
                                                        size="2">Filter:</font></td><td align="center" nowrap="1"><font face="Arial" size="2">
                                                        <xsl:value-of select="c1"/>s<sup>2</sup> +
                                                        <xsl:value-of select="c2"/>s + <xsl:value-of select="c3"/><br/><hr width="100%"/>
                                                        <xsl:value-of select="c4"/>s<sup>2</sup> + <xsl:value-of select="c5"/>s + <xsl:value-of select="c6"/>
                                                        </font></td></tr>
                                                    </table>
                                                </xsl:when>
                                                <xsl:when test="@type =  'WASHOUT_FILTER'" >
                                                    <i>Washout filter</i><br/>
                                                    <xsl:if test="input">Input: <i><xsl:value-of select="input"/></i><br/></xsl:if>
                                                </xsl:when>
                                                <xsl:when test="@type =  'PURE_GAIN'" >
                                                    <i>Gain</i><br/>
                                                    <xsl:if test="input">Input: <i><xsl:value-of select="input"/></i><br/></xsl:if>
                                                </xsl:when>
                                                <xsl:when test="@type =  'SCHEDULED_GAIN'" >
                                                    <i>Scheduled gain</i><br/>
                                                    <xsl:if test="input">Input: <i><xsl:value-of select="input"/></i><br/></xsl:if>
                                                </xsl:when>
                                                <xsl:when test="@type =  'AEROSURFACE_SCALE'" >
                                                    <i>Aerosurface scale</i><br/>
                                                    <xsl:if test="input">Input: <i><xsl:value-of select="input"/></i><br/></xsl:if>
                                                </xsl:when>
                                                <xsl:when test="@type =  'SUMMER'" >
                                                    <i>Summer</i><br/>
                                                    <xsl:for-each select="input">
                                                        Input: <i><xsl:value-of select="."/></i><br/>
                                                    </xsl:for-each>
                                                </xsl:when>
                                                <xsl:when test="@type =  'DEADBAND'" >
                                                    <i>Deadband</i><br/>
                                                    <xsl:if test="input">Input: <i><xsl:value-of select="input"/></i><br/></xsl:if>
                                                </xsl:when>
                                                <xsl:when test="@type =  'SWITCH'" >
                                                    <i>Switch</i><br/>
                                                    <xsl:if test="input">Input: <i><xsl:value-of select="input"/></i><br/></xsl:if>
                                                    <ul>
                                                    <xsl:for-each select="test">
                                                        <li>
                                                        <xsl:choose>
                                                            <xsl:when test="@logic='AND'">
                                                                If all of the following tests are true:
                                                            </xsl:when>
                                                            <xsl:when test="@logic='OR'">
                                                                If any of the following tests are true:
                                                            </xsl:when>
                                                            <xsl:when test="count(@logic)=0">
                                                                If all of the following tests are true:
                                                            </xsl:when>
                                                        </xsl:choose>
                                                        <pre style="font-size:90%"><xsl:value-of select="."/></pre>
                                                        then the switch value is: <xsl:value-of select="@value"/><br/></li>
                                                    </xsl:for-each>
                                                    <xsl:if test="default/@value"><li>Otherwise, the value is: <xsl:value-of
                                                    select="default/@value"/></li></xsl:if><br/>
                                                    </ul>
                                                </xsl:when>
                                                <xsl:when test="@type =  'KINEMAT'" >
                                                    <i>Kinematic</i><br/>
                                                    <xsl:if test="input">Input: <i><xsl:value-of select="input"/></i><br/></xsl:if>
                                                </xsl:when>
                                                <xsl:when test="@type =  'FUNCTION'" >
                                                    <i>Function</i><br/>
                                                    <xsl:if test="input">Input: <i><xsl:value-of select="input"/></i><br/></xsl:if>
                                                </xsl:when>
                                            </xsl:choose>
                                            <xsl:if test="clipto">
                                                <xsl:if test="clipto/min">Minimum limit: <xsl:value-of select="clipto/min"/><br/></xsl:if>
                                                <xsl:if test="clipto/max">Maximum limit: <xsl:value-of select="clipto/max"/></xsl:if>
                                            </xsl:if>
                                            <xsl:if test="output">Output to: <i><xsl:value-of select="output"/></i></xsl:if>

                                            </p></font>
                                    </xsl:for-each>
                                    <p/></td></tr>
                                </xsl:for-each>
                            </table>
                    </tr>

<tr><br/><font face="Arial" size="2">[<a href="#top">Top</a>]</font><p/></tr>
                    
                    <!-- AERODYNAMICS -->
                    <a name="aerodynamics"/>
                    <tr bgcolor="CCCCCC">
                            <table width="100%" style="font-family=Arial; font-size=90%" bgcolor="EEEEEE" cellpadding="0" cellspacing="0">
                                <tr><td colspan="3"><b>AERODYNAMICS</b></td></tr>
                                <tr><td colspan="3" height="3"><hr size="1"/></td></tr>
                                <tr><td colspan="3">The aerodynamics specification for this aircraft consists of <xsl:value-of select="count(aerodynamics/function)"/> function[s]
                                    and force components in <xsl:value-of select="count(aerodynamics/axis)"/> axes.<p/></td></tr>
                                <xsl:if test="count(aerodynamics/function) >0"><tr><td colspan="3"><b>Global Functions</b></td></tr></xsl:if>
                                <xsl:for-each select="aerodynamics/function">
                                    <tr><td width="300"><xsl:value-of select="@name"/></td><td colspan="2"><xsl:value-of select="description"/>
                                    <xsl:call-template name="process_function"/>
                                    </td></tr>
                                </xsl:for-each>
                                <xsl:for-each select="aerodynamics/axis">
                                    <tr><td colspan="3"><b><xsl:value-of select="@name"/> Axis</b></td></tr>
                                    <xsl:for-each select="function">
                                        <tr><td width="300" valign="top"><xsl:value-of select="@name"/></td><td colspan="2"><xsl:value-of select="description"/>
                                        <p/><xsl:call-template name="process_function"/></td></tr>
                                    </xsl:for-each>
                                </xsl:for-each>
                            </table>
                    </tr>
                    
<tr><br/><font face="Arial" size="2">[<a href="#top">Top</a>]</font><p/></tr>
                    
                    <!-- INPUT -->
                    <a name="input"/>
                    <tr bgcolor="BBBBBB">
                            <table width="100%" style="font-family=Arial; font-size=90%" bgcolor="EEEEEE" cellpadding="0" cellspacing="0">
                                <tr><td colspan="2"><b>INPUT</b></td></tr>
                                <tr><td colspan="2" height="3"><hr size="1"/></td></tr>
                                <tr><td>Input accepted on port <xsl:value-of select="input/@port"/>.</td></tr>
                            </table>
                    </tr>
                    
<tr><br/><font face="Arial" size="2">[<a href="#top">Top</a>]</font><p/></tr>
                    
                    <!-- OUTPUT -->
                    <a name="output"/>
                    <tr bgcolor="BBBBBB">
                            <table width="100%" style="font-family=Arial; font-size=90%" bgcolor="EEEEEE" cellpadding="0" cellspacing="0">
                                <tr><td colspan="2"><b>OUTPUT</b></td></tr>
                                <tr><td colspan="2" height="3"><hr size="1"/></td></tr>
                            </table>
                    </tr>
                </table>

<tr><br/><font face="Arial" size="2">[<a href="#top">Top</a>]</font><p/></tr>
                
            </body>
        </html>
    </xsl:template>
    
    <xsl:template name="process_function">
        <xsl:for-each select="product|table">
            <xsl:call-template name="process_products"/>
        </xsl:for-each>
    </xsl:template>

    <xsl:template name="process_products">
        [
        <xsl:for-each select="property">
            (<xsl:value-of select="."/>) 
        </xsl:for-each>
        <xsl:for-each select="table">
            <xsl:choose>
                <xsl:when test="@name">(<xsl:value-of select="@name"/>) </xsl:when>
                <xsl:otherwise>(table) </xsl:otherwise>
            </xsl:choose>
        </xsl:for-each>
        ]
    </xsl:template>

</xsl:stylesheet>

