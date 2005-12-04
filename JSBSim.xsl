<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
    <xsl:template match="/fdm_config">
        <html>
            <head>
                <title><xsl:value-of select="@name"/></title>
            </head>
            <body>
                <font face="Arial" size="3" color="224488"><b><xsl:value-of select="@name"/></b></font><br/>
                <font face="Arial" size="2">Configuration File Version: <xsl:value-of select="@version"/></font><br/>
                <hr width="100%"/>
                <table width="100%">
                    
                    <!-- FILEHEADER -->
                    <tr bgcolor="EEEEEE">
                        <font face="Arial" size="2">
                            <b>FILE INFORMATION</b><br/><br/>
                            <b>Author[s]: </b>
                            <xsl:for-each select="fileheader/author" >
                                <xsl:value-of select="."/><br/>
                            </xsl:for-each>
                            <b>File created: </b><xsl:value-of select="fileheader/filecreationdate"/><br/> 
                            <b>Description: </b><xsl:value-of select="fileheader/description"/><br/> 
                            <b>Model version information: </b><xsl:value-of select="fileheader/version"/><br/> 
                            <b>References: </b><br/>
                            <xsl:for-each select="fileheader/reference">
                                <xsl:value-of select="@refID"/>
                                , <i><xsl:value-of select="@title"/></i>
                                , <xsl:value-of select="@author"/>
                                , <xsl:value-of select="@date"/><br/>
                            </xsl:for-each>
                        </font>
                    </tr>

<tr><br/></tr>

                    <!-- METRICS -->
                    <tr bgcolor="DDEEFF">
                        <font face="Arial" size="2">
                            <b>METRICS</b><br/><br/>
                            <b>Wing span: </b><xsl:value-of select="metrics/wingspan"/>
                            <xsl:if test="metrics/wingspan/@unit != 0">(unit: <xsl:value-of select="metrics/wingspan/@unit"/>)</xsl:if><br/>
                            <b>Wing area: </b><xsl:value-of select="metrics/wingarea"/>
                            <xsl:if test="metrics/wingarea/@unit != 0">(unit: <xsl:value-of select="metrics/wingarea/@unit"/>)</xsl:if><br/>
                            <b>Chord: </b><xsl:value-of select="metrics/chord"/>
                            <xsl:if test="metrics/chord/@unit != 0">(unit: <xsl:value-of select="metrics/chord/@unit"/>)</xsl:if><br/>
                            <xsl:if test="metrics/htailarea">
                              <b>Horizontal tail area: </b><xsl:value-of select="metrics/htailarea"/>
                              <xsl:if test="metrics/htailarea/@unit != 0">(unit: <xsl:value-of select="metrics/htailarea/@unit"/>)</xsl:if><br/>
                            </xsl:if>
                            <xsl:if test="metrics/htailarm">
                                <b>Horizontal tail arm: </b><xsl:value-of select="metrics/htailarm"/>
                                <xsl:if test="metrics/htailarm/@unit != 0">(unit: <xsl:value-of select="metrics/htailarm/@unit"/>)</xsl:if><br/>
                            </xsl:if>
                            <xsl:if test="metrics/htailarea">
                                <b>Vertical tail area: </b><xsl:value-of select="metrics/vtailarea"/>
                                <xsl:if test="metrics/vtailarea/@unit != 0">(unit: <xsl:value-of select="metrics/vtailarea/@unit"/>)</xsl:if><br/>
                            </xsl:if>
                            <xsl:if test="metrics/vtailarm">
                                <b>Vertical tail arm: </b><xsl:value-of select="metrics/vtailarm"/>
                                <xsl:if test="metrics/vtailarm/@unit != 0">(unit: <xsl:value-of select="metrics/vtailarm/@unit"/>)</xsl:if><br/>
                            </xsl:if>
                            <xsl:for-each select="metrics/location">
                                <xsl:choose>
                                    <xsl:when test="@name = 'AERORP'">
                                      <b>Aerodynamic reference point location (<xsl:value-of select="@unit"/>): </b><xsl:value-of select="x"/>, <xsl:value-of select="y"/>, <xsl:value-of select="z"/><br/>
                                    </xsl:when>
                                    <xsl:when test="@name = 'VRP'">
                                        <b>Visual reference point location (<xsl:value-of select="@unit"/>): </b><xsl:value-of select="x"/>, <xsl:value-of select="y"/>, <xsl:value-of select="z"/><br/>
                                    </xsl:when>
                                    <xsl:when test="@name = 'EYEPOINT'">
                                        <b>Eyepoint location (<xsl:value-of select="@unit"/>): </b><xsl:value-of select="x"/>, <xsl:value-of select="y"/>, <xsl:value-of select="z"/><br/>
                                    </xsl:when>
                                    <xsl:otherwise><font color="FF0000"><xsl:value-of select="@name"/> is an invalid location!</font><br/></xsl:otherwise>
                                </xsl:choose>
                            </xsl:for-each>
                        </font>
                    </tr>
                    
<tr><br/></tr>
                    
                    <!-- MASS and BALANCE -->
                    <tr bgcolor="DDEEFF">
                        <font face="Arial" size="2">
                            <b>MASS AND BALANCE</b><br/><br/>
                            Moments of Inertia<br/>
                            <xsl:if test="mass_balance/ixx">
                                <b>Ixx (<xsl:value-of select="mass_balance/ixx/@unit"/>): </b><xsl:value-of select="mass_balance/ixx"/><br/>
                            </xsl:if>
                            <xsl:if test="mass_balance/iyy">
                                <b>Iyy (<xsl:value-of select="mass_balance/iyy/@unit"/>): </b><xsl:value-of select="mass_balance/iyy"/><br/>
                            </xsl:if>
                            <xsl:if test="mass_balance/izz">
                                <b>Izz (<xsl:value-of select="mass_balance/izz/@unit"/>): </b><xsl:value-of select="mass_balance/izz"/><br/>
                            </xsl:if>
                            <xsl:if test="mass_balance/ixy">
                                <b>Ixy (<xsl:value-of select="mass_balance/ixy/@unit"/>): </b><xsl:value-of select="mass_balance/ixy"/><br/>
                            </xsl:if>
                            <xsl:if test="mass_balance/ixz">
                                <b>Ixz (<xsl:value-of select="mass_balance/ixz/@unit"/>): </b><xsl:value-of select="mass_balance/ixz"/><br/>
                            </xsl:if>
                            <xsl:if test="mass_balance/iyz">
                                <b>Iyz (<xsl:value-of select="mass_balance/iyz/@unit"/>): </b><xsl:value-of select="mass_balance/iyz"/><br/>
                            </xsl:if>
                        </font>
                    </tr>
                    
<tr><br/></tr>
                    
                    <!-- GROUND REACTIONS -->
                    <tr bgcolor="DDEEFF">
                        <font face="Arial" size="2">
                            <b>GROUND REACTIONS</b><br/><br/>
                        </font>
                    </tr>
                    
<tr><br/></tr>
                    
                    <!-- PROPULSION -->
                    <tr bgcolor="DDEEFF">
                        <font face="Arial" size="2">
                            <b>PROPULSION</b><br/><br/>
                        </font>
                    </tr>
                    
<tr><br/></tr>
                    
                    <!-- AUTOPILOT -->
                    <tr bgcolor="FFDDDD">
                        <font face="Arial" size="2">
                            <b>AUTOPILOT</b><br/><br/>
                        </font></tr>                    
<!-- <tr><br/></tr> -->
                    <!-- FLIGHT CONTROL -->
                    <tr bgcolor="FFDDDD"><table width="100%" bgcolor="FFDDDD">
                            <th align="left" colspan="2"><font face="Arial" size="2"><b>FLIGHT CONTROL</b><br/></font></th>
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
                                                        <xsl:if test="count(c1)>0 and count(c2)>1"> + </xsl:if>
                                                        <xsl:if test="count(c2)>0"><xsl:value-of select="c2"/></xsl:if>
                                                        <br/><hr width="100%"/>
                                                        <xsl:if test="count(c3)>0"><xsl:value-of select="c3"/>s</xsl:if>
                                                        <xsl:if test="count(c3)>0 and count(c4)>1"> + </xsl:if>
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
                                                        <pre style="font-size:100%"><xsl:value-of select="."/></pre>
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

<tr><br/></tr>
                    
                    <!-- AERODYNAMICS -->
                    <tr bgcolor="CCCCCC">
                        <font face="Arial" size="2">
                            <b>AERODYNAMICS</b><br/><br/>
                        </font>
                    </tr>
                    
<tr><br/></tr>
                    
                    <!-- INPUT -->
                    <tr bgcolor="BBBBBB">
                        <font face="Arial" size="2">
                            <b>INPUT</b><br/><br/>
                        </font>
                    </tr>

<tr><br/></tr>
                    
                    <!-- OUTPUT -->
                    <tr bgcolor="AAAAAA">
                        <font face="Arial" size="2">
                            <b>OUTPUT</b><br/><br/>
                        </font>
                    </tr>
                </table>                
            </body>
        </html>
    </xsl:template>
</xsl:stylesheet>

