<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
    <xsl:output method="html" encoding="UTF-8"/>
    <xsl:template match="/fdm_config">
        <html>
            <head>
                <title>
                    <xsl:value-of select="@name"/>
                </title>
            </head>
            <body style="font-family:Arial;font-size:90%">
                <a name="top"/>
                <font face="Arial" size="3" color="224488">
                    <b>
                        <xsl:value-of select="@name"/>
                    </b>
                </font>
                <br/>
                <font face="Arial" size="2">Configuration File Version: <xsl:value-of
                        select="@version"/></font>
                <br/>
                <font face="Arial" size="2">Release level: <xsl:value-of select="@release"/></font>
                <br/>
                <xsl:if test="fileheader/license/licenseName">
                    <xsl:if test="fileheader/license/licenseURL">
                        <xsl:variable name="licenseURL" select="fileheader/license/licenseURL"/>
                        <font face="Arial" size="2">License: <a href="{$licenseURL}">
                                <xsl:value-of select="fileheader/license/licenseName"/>
                            </a>
                        </font>
                        <br/>
                    </xsl:if>
                </xsl:if>
                <hr width="100%"/>
                <font face="Arial" size="2">
                    <xsl:if test="fileheader">[<a href="#fileheader">File Information</a>] </xsl:if>
                    <xsl:if test="metrics">[<a href="#metrics">Metrics</a>] </xsl:if>
                    <xsl:if test="mass_balance">[<a href="#massbalance">Mass and Balance</a>] </xsl:if>
                    <xsl:if test="ground_reactions">[<a href="#groundreactions">Ground
                        Reactions</a>] </xsl:if>
                    <xsl:if test="buoyant_forces">[<a href="#buoyantforces">Buoyant Forces</a>] </xsl:if>
                    <xsl:if test="external_reactions">[<a href="#externalreactions">External
                            Reactions</a>] </xsl:if>
                    <xsl:if test="propulsion">[<a href="#propulsion">Propulsion</a>] </xsl:if>
                    <xsl:for-each select="system">
                        <xsl:if test="@name">
                            <xsl:variable name="sysname" select="@name"/> [<a
                                href="#system_{$sysname}">System: <xsl:value-of select="$sysname"
                            /></a>] </xsl:if>
                        <xsl:if test="@file">
                            <xsl:variable name="sysfile" select="@file"/> [<a
                                href="#system_{$sysfile}">System: <xsl:value-of select="$sysfile"
                            /></a>] </xsl:if>
                    </xsl:for-each>
                    <xsl:if test="autopilot">[<a href="#autopilot">Autopilot</a>] </xsl:if>
                    <xsl:if test="flight_control">[<a href="#flightcontrol">Flight Control</a>] </xsl:if>
                    <xsl:if test="aerodynamics">[<a href="#aerodynamics">Aerodynamics</a>] </xsl:if>
                    <xsl:if test="input">[<a href="#input">Input</a>] </xsl:if>
                    <xsl:if test="output">[<a href="#output">Output</a>]</xsl:if>
                </font>
                <p/>

                <table width="100%">
                    <xsl:if test="fileheader">
                        <!-- FILEHEADER -->
                        <a name="fileheader"/>
                        <tr bgcolor="EEEEEE">
                            <table width="100%" bgcolor="EEEEEE" cellpadding="0" cellspacing="0"
                                style="font-family:arial;font-size:90%">
                                <tr>
                                    <td colspan="4">
                                        <b>FILE INFORMATION</b>
                                    </td>
                                </tr>
                                <tr>
                                    <td colspan="4" height="3">
                                        <hr size="1"/>
                                    </td>
                                </tr>
                                <tr>
                                    <td valign="top">
                                        <b>Author[s]</b>
                                    </td>
                                    <td valign="top" colspan="3" align="left">
                                        <xsl:for-each select="fileheader/author">
                                            <xsl:value-of select="."/>, </xsl:for-each>
                                    </td>
                                </tr>
                                <tr>
                                    <td valign="top" width="150">
                                        <b>File created</b>
                                    </td>
                                    <td colspan="3" align="left">
                                        <xsl:value-of select="fileheader/filecreationdate"/>
                                    </td>
                                </tr>
                                <tr>
                                    <td valign="top" width="150">
                                        <b>Description</b>
                                    </td>
                                    <td colspan="3" align="left">
                                        <xsl:value-of select="fileheader/description"/>
                                    </td>
                                </tr>
                                <tr>
                                    <td valign="top" width="150">
                                        <b>Model version</b>
                                    </td>
                                    <td colspan="3">
                                        <xsl:value-of select="fileheader/version"/>
                                    </td>
                                </tr>
                                <xsl:if test="fileheader/reference">
                                    <tr>
                                        <td width="150">
                                            <b>References:</b>
                                            <br/>
                                        </td>
                                        <td/>
                                        <td/>
                                        <td/>
                                    </tr>
                                    <xsl:for-each select="fileheader/reference">
                                        <tr valign="top">
                                            <td>
                                                <xsl:value-of select="@refID"/>
                                            </td>
                                            <td>
                                                <i>
                                                  <xsl:value-of select="@title"/>
                                                </i>
                                            </td>
                                            <td>
                                                <xsl:value-of select="@author"/>
                                            </td>
                                            <td>
                                                <xsl:value-of select="@date"/>
                                            </td>
                                        </tr>
                                    </xsl:for-each>
                                </xsl:if>
                                <xsl:if test="fileheader/note">
                                    <tr>
                                        <td width="150">
                                            <b>Notes:</b>
                                            <br/>
                                        </td>
                                        <td/>
                                        <td/>
                                        <td/>
                                    </tr>
                                    <tr>
                                        <td colspan="4">
                                            <ol>
                                                <xsl:for-each select="fileheader/note">
                                                  <li>
                                                  <xsl:value-of select="."/>
                                                  </li>
                                                </xsl:for-each>
                                            </ol>
                                        </td>
                                    </tr>
                                </xsl:if>
                                <xsl:if test="fileheader/limitation">
                                    <tr>
                                        <td width="150">
                                            <b>Limitations:</b>
                                            <br/>
                                        </td>
                                        <td/>
                                        <td/>
                                        <td/>
                                    </tr>
                                    <tr>
                                        <td colspan="4">
                                            <ol>
                                                <xsl:for-each select="fileheader/limitation">
                                                  <li>
                                                  <xsl:value-of select="."/>
                                                  </li>
                                                </xsl:for-each>
                                            </ol>
                                        </td>
                                    </tr>
                                </xsl:if>
                            </table>
                        </tr>

                        <tr>
                            <br/>
                            <font face="Arial" size="2">[<a href="#top">Top</a>]</font>
                            <p/>
                        </tr>

                    </xsl:if>
                    <xsl:if test="metrics">

                        <!-- METRICS -->
                        <a name="metrics"/>
                        <tr bgcolor="DDEEFF">
                            <table width="100%" style="font-family:Arial; font-size:90%"
                                bgcolor="EEEEEE" cellpadding="0" cellspacing="0">
                                <tr>
                                    <td colspan="2">
                                        <b>METRICS</b>
                                    </td>
                                </tr>
                                <tr>
                                    <td colspan="2" height="3">
                                        <hr size="1"/>
                                    </td>
                                </tr>
                                <!-- wingspan -->
                                <tr>
                                    <td width="300">
                                        <b>Wing span <xsl:if test="metrics/wingspan/@unit"
                                                  >(<xsl:value-of select="metrics/wingspan/@unit"
                                                />)</xsl:if></b>
                                    </td>
                                    <td align="left">
                                        <xsl:value-of select="metrics/wingspan"/>
                                    </td>
                                </tr>
                                <!-- Wing area -->
                                <tr>
                                    <td width="300">
                                        <b>Wing area <xsl:if test="metrics/wingarea/@unit"
                                                  >(<xsl:value-of select="metrics/wingarea/@unit"
                                                />)</xsl:if></b>
                                    </td>
                                    <td>
                                        <xsl:value-of select="metrics/wingarea"/>
                                    </td>
                                </tr>
                                <!-- Chord -->
                                <tr>
                                    <td width="300">
                                        <b>Chord <xsl:if test="metrics/chord/@unit">(<xsl:value-of
                                                  select="metrics/chord/@unit"/>)</xsl:if></b>
                                    </td>
                                    <td>
                                        <xsl:value-of select="metrics/chord"/>
                                    </td>
                                </tr>
                                <!-- horizontal tail area -->
                                <xsl:if test="metrics/htailarea">
                                    <tr>
                                        <td width="300">
                                            <b>Horizontal tail area <xsl:if
                                                  test="metrics/htailarea/@unit">(<xsl:value-of
                                                  select="metrics/htailarea/@unit"
                                            />)</xsl:if></b>
                                        </td>
                                        <td>
                                            <xsl:value-of select="metrics/htailarea"/>
                                        </td>
                                    </tr>
                                </xsl:if>
                                <!-- Horizontal tail arm -->
                                <xsl:if test="metrics/htailarm">
                                    <tr>
                                        <td width="300">
                                            <b>Horizontal tail arm <xsl:if
                                                  test="metrics/htailarm/@unit">(<xsl:value-of
                                                  select="metrics/htailarm/@unit"
                                            />)</xsl:if></b>
                                        </td>
                                        <td>
                                            <xsl:value-of select="metrics/htailarm"/>
                                        </td>
                                    </tr>
                                </xsl:if>
                                <!-- Horizontal tail area -->
                                <xsl:if test="metrics/htailarea">
                                    <tr>
                                        <td width="300">
                                            <b>Vertical tail area <xsl:if
                                                  test="metrics/vtailarea/@unit">(<xsl:value-of
                                                  select="metrics/vtailarea/@unit"
                                            />)</xsl:if></b>
                                        </td>
                                        <td>
                                            <xsl:value-of select="metrics/vtailarea"/>
                                        </td>
                                    </tr>
                                </xsl:if>
                                <!-- Vertical tail arm -->
                                <xsl:if test="metrics/vtailarm">
                                    <tr>
                                        <td width="300">
                                            <b>Vertical tail arm <xsl:if
                                                  test="metrics/vtailarm/@unit">(<xsl:value-of
                                                  select="metrics/vtailarm/@unit"
                                            />)</xsl:if></b>
                                        </td>
                                        <td>
                                            <xsl:value-of select="metrics/vtailarm"/>
                                        </td>
                                    </tr>
                                </xsl:if>
                                <!-- locations -->
                                <xsl:for-each select="metrics/location">
                                    <xsl:choose>
                                        <xsl:when test="@name = 'AERORP'">
                                            <tr>
                                                <td width="300">
                                                  <b>Aerodynamic reference point location
                                                  (<xsl:value-of select="@unit"/>) </b>
                                                </td>
                                                <td><xsl:value-of select="x"/>, <xsl:value-of
                                                  select="y"/>, <xsl:value-of select="z"
                                                /></td>
                                            </tr>
                                        </xsl:when>
                                        <xsl:when test="@name = 'VRP'">
                                            <tr>
                                                <td width="300">
                                                  <b>Visual reference point location
                                                  (<xsl:value-of select="@unit"/>): </b>
                                                </td>
                                                <td><xsl:value-of select="x"/>, <xsl:value-of
                                                  select="y"/>, <xsl:value-of select="z"
                                                /></td>
                                            </tr>
                                        </xsl:when>
                                        <xsl:when test="@name = 'EYEPOINT'">
                                            <tr>
                                                <td width="300">
                                                  <b>Eyepoint location (<xsl:value-of
                                                  select="@unit"/>): </b>
                                                </td>
                                                <td><xsl:value-of select="x"/>, <xsl:value-of
                                                  select="y"/>, <xsl:value-of select="z"
                                                /></td>
                                            </tr>
                                        </xsl:when>
                                        <xsl:otherwise>
                                            <font color="FF0000">
                                                <tr>
                                                  <td colspan="2"><xsl:value-of select="@name"/>
                                                  is an invalid location!</td>
                                                </tr>
                                            </font>
                                            <br/>
                                        </xsl:otherwise>
                                    </xsl:choose>
                                </xsl:for-each>
                            </table>
                        </tr>

                        <tr>
                            <br/>
                            <font face="Arial" size="2">[<a href="#top">Top</a>]</font>
                            <p/>
                        </tr>

                    </xsl:if>
                    <xsl:if test="mass_balance">

                        <!-- MASS and BALANCE -->
                        <a name="massbalance"/>
                        <tr bgcolor="DDEEFF">
                            <table width="100%" style="font-family:Arial; font-size:90%"
                                bgcolor="EEEEEE" cellpadding="0" cellspacing="0">
                                <tr>
                                    <td colspan="2">
                                        <b>MASS AND BALANCE</b>
                                    </td>
                                </tr>
                                <tr>
                                    <td colspan="2" height="3">
                                        <hr size="1"/>
                                    </td>
                                </tr>
                                <tr>
                                    <td colspan="2">Moments of Inertia</td>
                                </tr>
                                <xsl:if test="mass_balance/ixx">
                                    <tr>
                                        <td width="300">
                                            <b>Ixx <xsl:if test="mass_balance/ixx/@unit"
                                                  >(<xsl:value-of
                                                  select="mass_balance/ixx/@unit"
                                            />)</xsl:if></b>
                                        </td>
                                        <td>
                                            <xsl:value-of select="mass_balance/ixx"/>
                                        </td>
                                    </tr>
                                </xsl:if>
                                <xsl:if test="mass_balance/iyy">
                                    <tr>
                                        <td width="300">
                                            <b>Iyy <xsl:if test="mass_balance/iyy/@unit"
                                                  >(<xsl:value-of
                                                  select="mass_balance/iyy/@unit"
                                            />)</xsl:if></b>
                                        </td>
                                        <td>
                                            <xsl:value-of select="mass_balance/iyy"/>
                                        </td>
                                    </tr>
                                </xsl:if>
                                <xsl:if test="mass_balance/izz">
                                    <tr>
                                        <td width="300">
                                            <b>Izz <xsl:if test="mass_balance/izz/@unit"
                                                  >(<xsl:value-of
                                                  select="mass_balance/izz/@unit"
                                            />)</xsl:if></b>
                                        </td>
                                        <td>
                                            <xsl:value-of select="mass_balance/izz"/>
                                        </td>
                                    </tr>
                                </xsl:if>
                                <xsl:if test="mass_balance/ixy">
                                    <tr>
                                        <td width="300">
                                            <b>Ixy <xsl:if test="mass_balance/ixy/@unit"
                                                  >(<xsl:value-of
                                                  select="mass_balance/ixy/@unit"
                                            />)</xsl:if></b>
                                        </td>
                                        <td>
                                            <xsl:value-of select="mass_balance/ixy"/>
                                        </td>
                                    </tr>
                                </xsl:if>
                                <xsl:if test="mass_balance/ixz">
                                    <tr>
                                        <td width="300">
                                            <b>Ixz <xsl:if test="mass_balance/ixz/@unit"
                                                  >(<xsl:value-of
                                                  select="mass_balance/ixz/@unit"
                                            />)</xsl:if></b>
                                        </td>
                                        <td>
                                            <xsl:value-of select="mass_balance/ixz"/>
                                        </td>
                                    </tr>
                                </xsl:if>
                                <xsl:if test="mass_balance/iyz">
                                    <tr>
                                        <td width="300">
                                            <b>Iyz <xsl:if test="mass_balance/iyz/@unit"
                                                  >(<xsl:value-of
                                                  select="mass_balance/iyz/@unit"
                                            />)</xsl:if></b>
                                        </td>
                                        <td>
                                            <xsl:value-of select="mass_balance/iyz"/>
                                        </td>
                                    </tr>
                                </xsl:if>
                                <xsl:if test="mass_balance/emptywt">
                                    <tr>
                                        <td width="300">
                                            <b>Empty Weight <xsl:if
                                                  test="mass_balance/emptywt/@unit">(<xsl:value-of
                                                  select="mass_balance/emptywt/@unit"
                                                />)</xsl:if></b>
                                        </td>
                                        <td>
                                            <xsl:value-of select="mass_balance/emptywt"/>
                                        </td>
                                    </tr>
                                </xsl:if>
                                <xsl:if test="mass_balance/location">
                                    <tr>
                                        <td width="300">
                                            <b>Center of Gravity (CG) Location: </b>
                                            <xsl:if test="mass_balance/location/@unit"
                                                  >(<xsl:value-of
                                                  select="mass_balance/location/@unit"/>)</xsl:if>
                                        </td>
                                        <td>
                                            <xsl:value-of select="mass_balance/location/x"/>,
                                                <xsl:value-of select="mass_balance/location/y"/>,
                                                <xsl:value-of select="mass_balance/location/z"/>
                                        </td>
                                    </tr>
                                </xsl:if>
                                <xsl:for-each select="mass_balance/pointmass">
                                    <tr>
                                        <td width="300" valign="top">
                                            <b>
                                                <xsl:value-of select="@name"/>
                                            </b>
                                        </td>
                                        <td valign="top"><u>Weight<xsl:if test="weight/@unit">
                                                  (<xsl:value-of select="weight/@unit"
                                                />)</xsl:if></u>: <xsl:value-of select="weight"/><br/>
                                            <u>Position <xsl:if test="location/@unit">(<xsl:value-of
                                                  select="location/@unit"/>)</xsl:if></u>:
                                                <xsl:value-of select="location/x"/>, <xsl:value-of
                                                select="location/y"/>, <xsl:value-of
                                                select="location/z"/>
                                        </td>
                                    </tr>
                                </xsl:for-each>
                            </table>
                        </tr>

                        <tr>
                            <br/>
                            <font face="Arial" size="2">[<a href="#top">Top</a>]</font>
                            <p/>
                        </tr>

                    </xsl:if>
                    <xsl:if test="ground_reactions">

                        <!-- GROUND REACTIONS -->
                        <a name="groundreactions"/>
                        <tr bgcolor="DDEEFF">
                            <table width="100%" style="font-family:Arial; font-size:90%"
                                bgcolor="EEEEEE" cellpadding="0" cellspacing="0">
                                <tr>
                                    <td colspan="2">
                                        <b>GROUND REACTIONS</b>
                                    </td>
                                </tr>
                                <tr>
                                    <td colspan="2" height="3">
                                        <hr size="1"/>
                                    </td>
                                </tr>
                                <xsl:for-each select="ground_reactions/contact">
                                    <xsl:if test="@type='BOGEY'">
                                        <tr>
                                            <td valign="top">Bogey: <xsl:value-of select="@name"/>
                                            </td>
                                            <td>
                                                <u>Position <xsl:if test="location/@unit"
                                                  >(<xsl:value-of select="location/@unit"
                                                  />)</xsl:if></u>: <xsl:value-of
                                                  select="location/x"/>, <xsl:value-of
                                                  select="location/y"/>, <xsl:value-of
                                                  select="location/z"/><br/>
                                                <u>Static friction coefficient</u>: <xsl:value-of
                                                  select="static_friction"/><br/>
                                                <u>Dynamic friction coefficient</u>: <xsl:value-of
                                                  select="dynamic_friction"/><br/>
                                                <u>Rolling friction coefficient</u>: <xsl:value-of
                                                  select="rolling_friction"/><br/>
                                                <u>Spring constant<xsl:if test="spring_coeff/@unit">
                                                  (<xsl:value-of
                                                  select="spring_coeff/@unit"
                                                />)</xsl:if></u>: <xsl:value-of
                                                  select="spring_coeff"/><br/>
                                                <u>Damping constant<xsl:if
                                                  test="damping_coeff/@unit"> (<xsl:value-of
                                                  select="damping_coeff/@unit"
                                                />)</xsl:if></u>: <xsl:value-of
                                                  select="damping_coeff"/><br/>
                                                <u>Maximum steering angle<xsl:if
                                                  test="max_steer/@unit"> (<xsl:value-of
                                                  select="max_steer/@unit"
                                                />)</xsl:if></u>: <xsl:value-of select="max_steer"
                                                /><br/>
                                            </td>
                                        </tr>
                                    </xsl:if>
                                </xsl:for-each>
                                <xsl:for-each select="ground_reactions/contact">
                                    <xsl:if test="@type=STRUCTURE">
                                        <tr>
                                            <td>Contact point: <xsl:value-of select="@name"/></td>
                                            <td>Stuff</td>
                                        </tr>
                                    </xsl:if>
                                </xsl:for-each>
                            </table>
                        </tr>

                        <tr>
                            <br/>
                            <font face="Arial" size="2">[<a href="#top">Top</a>]</font>
                            <p/>
                        </tr>

                    </xsl:if>
                    <xsl:if test="propulsion">

                        <!-- PROPULSION -->
                        <a name="propulsion"/>
                        <tr bgcolor="DDEEFF">
                            <table width="100%" style="font-family:Arial; font-size:90%"
                                bgcolor="DDEEFF" cellpadding="0" cellspacing="0">
                                <tr>
                                    <td colspan="2">
                                        <b>PROPULSION</b>
                                    </td>
                                </tr>
                                <tr>
                                    <td colspan="2" height="3">
                                        <hr size="1"/>
                                    </td>
                                </tr>
                                <xsl:for-each select="propulsion/engine">
                                    <tr>
                                        <td valign="top"  width="25%" nowrap="nowrap">
                                        <font face="Arial" size="2">
                                            <b>Engine: </b>
                                            [Defined in file: <xsl:value-of select="@file"/>.xml]
                                        </font>
                                        </td>
                                        <td width="10"> </td>
                                    <td align="left">
                                        <font face="Arial" size="2"/>
                                        <b>Engine location</b>: [<xsl:value-of select="location/x"/>,
                                                                      <xsl:value-of select="location/y"/>,
                                                                      <xsl:value-of select="location/z"/>] (Unit: <xsl:value-of select="location/@unit"/>)<br/>
                                         <b>Thruster</b>: [Defined in file: <xsl:value-of select="thruster/@file"/>.xml]
                                                                      
                                    </td>
                                    </tr>
                                </xsl:for-each>
                            </table>
                        </tr>

                        <tr>
                            <br/>
                            <font face="Arial" size="2">[<a href="#top">Top</a>]</font>
                            <p/>
                        </tr>

                    </xsl:if>

                    <!-- SYSTEM -->
                    <xsl:for-each select="system">
                        <xsl:if test="@name">
                            <xsl:variable name="sysname" select="@name"/>
                            <a name="system_{$sysname}"/>
                            <tr bgcolor="FFDDDD">
                                <table width="100%" style="font-family:Arial; font-size:90%"
                                    bgcolor="FFDDDD" cellpadding="0" cellspacing="0">
                                    <tr>
                                        <td colspan="2">
                                            <b>System: <xsl:value-of select="$sysname"/></b>
                                        </td>
                                    </tr>
                                    <tr>
                                        <td colspan="2" height="3">
                                            <hr size="1"/>
                                        </td>
                                    </tr>
                                <xsl:for-each select="channel">
                                    <tr>
                                        <td valign="top">
                                            <font face="Arial" size="2">
                                                <b>Channel </b>
                                                <xsl:value-of select="@name"/>
                                            </font>
                                        </td>
                                        <xsl:variable name="ucletters">ABCDEFGHIJKLMNOPQRSTUVWXYZ </xsl:variable>
                                        <xsl:variable name="lcletters">abcdefghijklmnopqrstuvwxyz-</xsl:variable>

                                        <td>
                                            <xsl:for-each select="child::*">
                                                <font face="Arial" size="2">
                                                  <p><b>Component: </b><xsl:value-of
                                                  select="@name"/>, Type:
                                                  <xsl:choose>
                                                      <!-- LAG FILTER -->
                                                  <xsl:when test="name() = 'lag_filter'">
                                                  <i>Lag filter</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  <table>
                                                  <tr>
                                                  <td nowrap="1"
                                                  valign="center"
                                                  align="left">
                                                  <font face="Arial"
                                                  size="2">
                                                  Filter:</font>
                                                  </td>
                                                  <td align="center"
                                                  nowrap="1">
                                                  <font face="Arial"
                                                  size="2">
                                                  <xsl:value-of
                                                  select="c1"/><hr
                                                  width="100%"/> s
                                                  + <xsl:value-of
                                                  select="c1"/>
                                                  </font>
                                                  </td>
                                                  </tr>
                                                  </table>
                                                  </xsl:when>
                                                      <!-- INTEGRATOR -->
                                                  <xsl:when test="name() = 'integrator'">
                                                  <i>Integrator</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  <table>
                                                  <tr>
                                                  <td nowrap="1"
                                                  valign="center"
                                                  align="left">
                                                  <font face="Arial"
                                                  size="2">
                                                  Filter:</font>
                                                  </td>
                                                  <td align="center"
                                                  nowrap="1">
                                                  <font face="Arial"
                                                  size="2">
                                                  <xsl:value-of
                                                  select="c1"/><hr
                                                  width="100%"/>
                                                  s</font>
                                                  </td>
                                                  </tr>
                                                  </table>
                                                  <xsl:if test="trigger"> Trigger
                                                  property: <xsl:value-of
                                                  select="trigger"/>
                                                  </xsl:if>
                                                  </xsl:when>
                                                      <!-- LEAD LAG FILTER -->
                                                  <xsl:when
                                                  test="name() =  'lead_lag_filter'">
                                                  <i>Lead-lag filter</i><br/>
                                                  Component property name:
                                                  <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  <table>
                                                  <tr>
                                                  <td nowrap="1"
                                                  valign="center"
                                                  align="left">
                                                  <font face="Arial"
                                                  size="2"
                                                  >Filter:</font>
                                                  </td>
                                                  <td align="center"
                                                  nowrap="1">
                                                  <font face="Arial"
                                                  size="2">
                                                  <xsl:if
                                                  test="count(c1)>0"
                                                  ><xsl:value-of
                                                  select="c1"
                                                  />s</xsl:if>
                                                  <xsl:if
                                                  test="count(c1)=1 and count(c2)=1"
                                                  > + </xsl:if>
                                                  <xsl:if
                                                  test="count(c2)>0">
                                                  <xsl:value-of
                                                  select="c2"
                                                  />
                                                  </xsl:if>
                                                  <hr width="100%"/>
                                                  <xsl:if
                                                  test="count(c3)>0"
                                                  ><xsl:value-of
                                                  select="c3"
                                                  />s</xsl:if>
                                                  <xsl:if
                                                  test="count(c3)=1 and count(c4)=1"
                                                  > + </xsl:if>
                                                  <xsl:if
                                                  test="count(c4)>0">
                                                  <xsl:value-of
                                                  select="c4"
                                                  />
                                                  </xsl:if>
                                                  </font>
                                                  </td>
                                                  </tr>
                                                  </table>
                                                  </xsl:when>
                                                      <!-- SECOND ORDER FILTER -->
                                                      <xsl:when
                                                  test="name() =  'second_order_filter'">
                                                  <i>Second order filter</i><br/>
                                                  Component property name:
                                                  <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  <table>
                                                  <tr>
                                                  <td nowrap="1"
                                                  valign="center"
                                                  align="left">
                                                  <font face="Arial"
                                                  size="2"
                                                  >Filter:</font>
                                                  </td>
                                                  <td align="center"
                                                  nowrap="1">
                                                  <font face="Arial"
                                                  size="2">
                                                  <xsl:value-of
                                                  select="c1"
                                                  />s<sup>2</sup>
                                                  + <xsl:value-of
                                                  select="c2"/>s +
                                                  <xsl:value-of
                                                  select="c3"/><hr
                                                  width="100%"/>
                                                  <xsl:value-of
                                                  select="c4"
                                                  />s<sup>2</sup>
                                                  + <xsl:value-of
                                                  select="c5"/>s +
                                                  <xsl:value-of
                                                  select="c6"/>
                                                  </font>
                                                  </td>
                                                  </tr>
                                                  </table>
                                                      </xsl:when>
                                                      <!-- WASHOUT FILTER -->
                                                  <xsl:when
                                                  test="name() = 'washout_filter'">
                                                  <i>Washout filter</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  <table>
                                                  <tr>
                                                  <td nowrap="1"
                                                  valign="center"
                                                  align="left">
                                                  <font face="Arial"
                                                  size="2">
                                                  Filter:</font>
                                                  </td>
                                                  <td align="center"
                                                  nowrap="1">
                                                  <font face="Arial"
                                                  size="2"> s<hr
                                                  width="100%"/> s
                                                  + <xsl:value-of
                                                  select="c1"/>
                                                  </font>
                                                  </td>
                                                  </tr>
                                                  </table>
                                                  </xsl:when>
                                                      <!-- PURE GAIN -->
                                                  <xsl:when test="name() =  'pure_gain'">
                                                  <i>Gain</i><br/> Component property
                                                  name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  </xsl:when>
                                                      <!-- SCHEDULED GAIN -->
                                                  <xsl:when
                                                  test="name() =  'scheduled_gain'">
                                                  <i>Scheduled gain</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  </xsl:when>
                                                      <!-- AEROSURFACE SCALE -->
                                                  <xsl:when
                                                  test="name() =  'aerosurface_scale'">
                                                  <i>Aerosurface scale</i><br/>
                                                  Component property name:
                                                  <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  </xsl:when>
                                                      <!-- SUMMER -->
                                                  <xsl:when test="name() =  'summer'">
                                                  <i>Summer</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:for-each select="input"> Input: <i>
                                                  <xsl:value-of select="."/>
                                                  </i><br/>
                                                  </xsl:for-each>
                                                  </xsl:when>
                                                      <!-- DEADBAND -->
                                                  <xsl:when test="name() =  'deadband'">
                                                  <i>Deadband</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  </xsl:when>
                                                      <!-- SWITCH -->
                                                  <xsl:when test="name() =  'switch'">
                                                  <i>Switch</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  <ul>
                                                  <xsl:for-each select="test">
                                                  <li>
                                                  <xsl:choose>
                                                  <xsl:when
                                                  test="@logic='AND'"
                                                  > If all of the
                                                  following tests
                                                  are true: </xsl:when>
                                                  <xsl:when
                                                  test="@logic='OR'"
                                                  > If any of the
                                                  following tests
                                                  are true: </xsl:when>
                                                  <xsl:when
                                                  test="count(@logic)=0"
                                                  > If all of the
                                                  following tests
                                                  are true:
                                                  </xsl:when>
                                                  </xsl:choose>
                                                  <pre style="font-size:90%"><xsl:value-of select="."/></pre>
                                                  then the switch value
                                                  is: <xsl:value-of
                                                  select="@value"
                                                  /><br/></li>
                                                  </xsl:for-each>
                                                  <xsl:if test="default/@value">
                                                  <li>Otherwise, the value is:
                                                  <xsl:value-of
                                                  select="default/@value"
                                                  /></li>
                                                  </xsl:if>
                                                  <br/>
                                                  </ul>
                                                  </xsl:when>
                                                      <!-- KINEMATIC -->
                                                  <xsl:when test="name() =  'kinematic'">
                                                  <i>Kinematic</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  </xsl:when>
                                                      <!-- FCS FUNCTION -->
                                                  <xsl:when
                                                  test="name() =  'fcs_function'">
                                                  <i>Function</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  <xsl:call-template
                                                  name="process_function"/>
                                                  </xsl:when>
                                                      <!-- PID controller -->
                                                      <xsl:when test="name() = 'pid'">
                                                          <i>PID Controller</i><br/> Component
                                                          property name: <i>fcs/<xsl:value-of
                                                              select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                          /></i><br/>
                                                          <xsl:if test="input">Input: <i>
                                                              <xsl:value-of select="input"/>
                                                          </i><br/></xsl:if>
                                                          <table>
                                                              <tr>
                                                                  <td nowrap="1" valign="center" align="left">
                                                                      <font face="Arial" size="2">Proportional gain (kp): <xsl:value-of select="kp"/></font><br/>
                                                                      <font face="Arial" size="2">Integral gain (ki): <xsl:value-of select="ki"/></font><br/>
                                                                      <font face="Arial" size="2">Derivative gain (kd): <xsl:value-of select="kd"/></font>
                                                                  </td>
                                                              </tr>
                                                          </table>
                                                          <xsl:if test="trigger"> Trigger
                                                              property: <xsl:value-of
                                                                  select="trigger"/>
                                                          </xsl:if>
                                                      </xsl:when>
                                                  </xsl:choose>
                                                  <xsl:if test="clipto">
                                                  <xsl:if test="clipto/min">Minimum limit:
                                                  <xsl:value-of
                                                  select="clipto/min"/><br/></xsl:if>
                                                  <xsl:if test="clipto/max">Maximum limit:
                                                  <xsl:value-of
                                                  select="clipto/max"
                                                  /><br/></xsl:if>
                                                  </xsl:if>
                                                  <xsl:if test="output">Output to: <i>
                                                  <xsl:value-of select="output"/>
                                                  </i></xsl:if>
                                                  </p>
                                                </font>
                                            </xsl:for-each>
                                            <p/>
                                        </td>
                                    </tr>
                                </xsl:for-each>
                                </table>
                            </tr>
                        </xsl:if>
                        <xsl:if test="@file">
                            <xsl:variable name="sysfile" select="@file"/>
                            <a name="system_{$sysfile}"/>
                            <tr bgcolor="FFDDDD">
                                <table width="100%" style="font-family:Arial; font-size:90%"
                                    bgcolor="FFDDDD" cellpadding="0" cellspacing="0">
                                    <tr>
                                        <td colspan="2">
                                            <b>System: [Defined in file: <xsl:value-of select="$sysfile"/>.xml]</b>
                                        </td>
                                    </tr>
                                    <tr>
                                        <td colspan="2" height="3">
                                            <hr size="1"/>
                                        </td>
                                    </tr>
                                </table>
                            </tr>
                        </xsl:if>
                        <tr>
                            <br/>
                            <font face="Arial" size="2">[<a href="#top">Top</a>]</font>
                            <p/>
                        </tr>
                    </xsl:for-each>

                    <!-- AUTOPILOT -->
                    <xsl:if test="autopilot">
                        <xsl:if test="autopilot/@name">
                            <a name="autopilot"/>
                            <tr bgcolor="FFDDDD">
                                <table width="100%" style="font-family:Arial; font-size:90%"
                                    bgcolor="FFDDDD" cellpadding="0" cellspacing="0">
                                    <tr>
                                        <td colspan="2">
                                            <b>Autopilot: <xsl:value-of select="autopilot/@name"/></b>
                                        </td>
                                    </tr>
                                    <tr>
                                        <td colspan="2" height="3">
                                            <hr size="1"/>
                                        </td>
                                    </tr>
                                <xsl:for-each select="autopilot/channel">
                                    <tr>
                                        <td valign="top">
                                            <font face="Arial" size="2">
                                                <b>Channel </b>
                                                <xsl:value-of select="@name"/>
                                            </font>
                                        </td>
                                        <xsl:variable name="ucletters">ABCDEFGHIJKLMNOPQRSTUVWXYZ </xsl:variable>
                                        <xsl:variable name="lcletters">abcdefghijklmnopqrstuvwxyz-</xsl:variable>

                                        <td>
                                            <xsl:for-each select="child::*">
                                                <font face="Arial" size="2">
                                                  <p><b>Component: </b><xsl:value-of
                                                  select="@name"/>, Type:
                                                  <xsl:choose>
                                                      <!-- LAG FILTER -->
                                                  <xsl:when test="name() = 'lag_filter'">
                                                  <i>Lag filter</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  <table>
                                                  <tr>
                                                  <td nowrap="1"
                                                  valign="center"
                                                  align="left">
                                                  <font face="Arial"
                                                  size="2">
                                                  Filter:</font>
                                                  </td>
                                                  <td align="center"
                                                  nowrap="1">
                                                  <font face="Arial"
                                                  size="2">
                                                  <xsl:value-of
                                                  select="c1"/><hr
                                                  width="100%"/> s
                                                  + <xsl:value-of
                                                  select="c1"/>
                                                  </font>
                                                  </td>
                                                  </tr>
                                                  </table>
                                                  </xsl:when>
                                                      <!-- INTEGRATOR -->
                                                  <xsl:when test="name() = 'integrator'">
                                                  <i>Integrator</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  <table>
                                                  <tr>
                                                  <td nowrap="1"
                                                  valign="center"
                                                  align="left">
                                                  <font face="Arial"
                                                  size="2">
                                                  Filter:</font>
                                                  </td>
                                                  <td align="center"
                                                  nowrap="1">
                                                  <font face="Arial"
                                                  size="2">
                                                  <xsl:value-of
                                                  select="c1"/><hr
                                                  width="100%"/>
                                                  s</font>
                                                  </td>
                                                  </tr>
                                                  </table>
                                                  <xsl:if test="trigger"> Trigger
                                                  property: <xsl:value-of
                                                  select="trigger"/>
                                                  </xsl:if>
                                                  </xsl:when>
                                                      <!-- LEAD LAG FILTER -->
                                                  <xsl:when
                                                  test="name() =  'lead_lag_filter'">
                                                  <i>Lead-lag filter</i><br/>
                                                  Component property name:
                                                  <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  <table>
                                                  <tr>
                                                  <td nowrap="1"
                                                  valign="center"
                                                  align="left">
                                                  <font face="Arial"
                                                  size="2"
                                                  >Filter:</font>
                                                  </td>
                                                  <td align="center"
                                                  nowrap="1">
                                                  <font face="Arial"
                                                  size="2">
                                                  <xsl:if
                                                  test="count(c1)>0"
                                                  ><xsl:value-of
                                                  select="c1"
                                                  />s</xsl:if>
                                                  <xsl:if
                                                  test="count(c1)=1 and count(c2)=1"
                                                  > + </xsl:if>
                                                  <xsl:if
                                                  test="count(c2)>0">
                                                  <xsl:value-of
                                                  select="c2"
                                                  />
                                                  </xsl:if>
                                                  <hr width="100%"/>
                                                  <xsl:if
                                                  test="count(c3)>0"
                                                  ><xsl:value-of
                                                  select="c3"
                                                  />s</xsl:if>
                                                  <xsl:if
                                                  test="count(c3)=1 and count(c4)=1"
                                                  > + </xsl:if>
                                                  <xsl:if
                                                  test="count(c4)>0">
                                                  <xsl:value-of
                                                  select="c4"
                                                  />
                                                  </xsl:if>
                                                  </font>
                                                  </td>
                                                  </tr>
                                                  </table>
                                                  </xsl:when>
                                                      <!-- SECOND ORDER FILTER -->
                                                      <xsl:when
                                                  test="name() =  'second_order_filter'">
                                                  <i>Second order filter</i><br/>
                                                  Component property name:
                                                  <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  <table>
                                                  <tr>
                                                  <td nowrap="1"
                                                  valign="center"
                                                  align="left">
                                                  <font face="Arial"
                                                  size="2"
                                                  >Filter:</font>
                                                  </td>
                                                  <td align="center"
                                                  nowrap="1">
                                                  <font face="Arial"
                                                  size="2">
                                                  <xsl:value-of
                                                  select="c1"
                                                  />s<sup>2</sup>
                                                  + <xsl:value-of
                                                  select="c2"/>s +
                                                  <xsl:value-of
                                                  select="c3"/><hr
                                                  width="100%"/>
                                                  <xsl:value-of
                                                  select="c4"
                                                  />s<sup>2</sup>
                                                  + <xsl:value-of
                                                  select="c5"/>s +
                                                  <xsl:value-of
                                                  select="c6"/>
                                                  </font>
                                                  </td>
                                                  </tr>
                                                  </table>
                                                      </xsl:when>
                                                      <!-- WASHOUT FILTER -->
                                                  <xsl:when
                                                  test="name() = 'washout_filter'">
                                                  <i>Washout filter</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  <table>
                                                  <tr>
                                                  <td nowrap="1"
                                                  valign="center"
                                                  align="left">
                                                  <font face="Arial"
                                                  size="2">
                                                  Filter:</font>
                                                  </td>
                                                  <td align="center"
                                                  nowrap="1">
                                                  <font face="Arial"
                                                  size="2"> s<hr
                                                  width="100%"/> s
                                                  + <xsl:value-of
                                                  select="c1"/>
                                                  </font>
                                                  </td>
                                                  </tr>
                                                  </table>
                                                  </xsl:when>
                                                      <!-- PURE GAIN -->
                                                  <xsl:when test="name() =  'pure_gain'">
                                                  <i>Gain</i><br/> Component property
                                                  name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  </xsl:when>
                                                      <!-- SCHEDULED GAIN -->
                                                  <xsl:when
                                                  test="name() =  'scheduled_gain'">
                                                  <i>Scheduled gain</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  </xsl:when>
                                                      <!-- AEROSURFACE SCALE -->
                                                  <xsl:when
                                                  test="name() =  'aerosurface_scale'">
                                                  <i>Aerosurface scale</i><br/>
                                                  Component property name:
                                                  <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  </xsl:when>
                                                      <!-- SUMMER -->
                                                  <xsl:when test="name() =  'summer'">
                                                  <i>Summer</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:for-each select="input"> Input: <i>
                                                  <xsl:value-of select="."/>
                                                  </i><br/>
                                                  </xsl:for-each>
                                                  </xsl:when>
                                                      <!-- DEADBAND -->
                                                  <xsl:when test="name() =  'deadband'">
                                                  <i>Deadband</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  </xsl:when>
                                                      <!-- SWITCH -->
                                                  <xsl:when test="name() =  'switch'">
                                                  <i>Switch</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  <ul>
                                                  <xsl:for-each select="test">
                                                  <li>
                                                  <xsl:choose>
                                                  <xsl:when
                                                  test="@logic='AND'"
                                                  > If all of the
                                                  following tests
                                                  are true: </xsl:when>
                                                  <xsl:when
                                                  test="@logic='OR'"
                                                  > If any of the
                                                  following tests
                                                  are true: </xsl:when>
                                                  <xsl:when
                                                  test="count(@logic)=0"
                                                  > If all of the
                                                  following tests
                                                  are true:
                                                  </xsl:when>
                                                  </xsl:choose>
                                                  <pre style="font-size:90%"><xsl:value-of select="."/></pre>
                                                  then the switch value
                                                  is: <xsl:value-of
                                                  select="@value"
                                                  /><br/></li>
                                                  </xsl:for-each>
                                                  <xsl:if test="default/@value">
                                                  <li>Otherwise, the value is:
                                                  <xsl:value-of
                                                  select="default/@value"
                                                  /></li>
                                                  </xsl:if>
                                                  <br/>
                                                  </ul>
                                                  </xsl:when>
                                                      <!-- KINEMATIC -->
                                                  <xsl:when test="name() =  'kinematic'">
                                                  <i>Kinematic</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  </xsl:when>
                                                      <!-- FCS FUNCTION -->
                                                  <xsl:when
                                                  test="name() =  'fcs_function'">
                                                  <i>Function</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  <xsl:call-template
                                                  name="process_function"/>
                                                  </xsl:when>
                                                      <!-- PID controller -->
                                                      <xsl:when test="name() = 'pid'">
                                                          <i>PID Controller</i><br/> Component
                                                          property name: <i>fcs/<xsl:value-of
                                                              select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                          /></i><br/>
                                                          <xsl:if test="input">Input: <i>
                                                              <xsl:value-of select="input"/>
                                                          </i><br/></xsl:if>
                                                          <table>
                                                              <tr>
                                                                  <td nowrap="1" valign="center" align="left">
                                                                      <font face="Arial" size="2">Proportional gain (kp): <xsl:value-of select="kp"/></font><br/>
                                                                      <font face="Arial" size="2">Integral gain (ki): <xsl:value-of select="ki"/></font><br/>
                                                                      <font face="Arial" size="2">Derivative gain (kd): <xsl:value-of select="kd"/></font>
                                                                  </td>
                                                              </tr>
                                                          </table>
                                                          <xsl:if test="trigger"> Trigger
                                                              property: <xsl:value-of
                                                                  select="trigger"/>
                                                          </xsl:if>
                                                      </xsl:when>
                                                  </xsl:choose>
                                                  <xsl:if test="clipto">
                                                  <xsl:if test="clipto/min">Minimum limit:
                                                  <xsl:value-of
                                                  select="clipto/min"/><br/></xsl:if>
                                                  <xsl:if test="clipto/max">Maximum limit:
                                                  <xsl:value-of
                                                  select="clipto/max"
                                                  /><br/></xsl:if>
                                                  </xsl:if>
                                                  <xsl:if test="output">Output to: <i>
                                                  <xsl:value-of select="output"/>
                                                  </i></xsl:if>
                                                  </p>
                                                </font>
                                            </xsl:for-each>
                                            <p/>
                                        </td>
                                    </tr>
                                </xsl:for-each>
                                </table>
                            </tr>
                        </xsl:if>
                        <xsl:if test="autopilot/@file">
                            <a name="autopilot"/>
                            <tr bgcolor="FFDDDD">
                                <table width="100%" style="font-family:Arial; font-size:90%"
                                    bgcolor="FFDDDD" cellpadding="0" cellspacing="0">
                                    <tr>
                                        <td colspan="2">
                                            <b>Autopilot: [Defined in file: <xsl:value-of select="autopilot/@file"/>.xml]</b>
                                        </td>
                                    </tr>
                                    <tr>
                                        <td colspan="2" height="3">
                                            <hr size="1"/>
                                        </td>
                                    </tr>
                                </table>
                            </tr>
                        </xsl:if>
                        <tr>
                            <br/>
                            <font face="Arial" size="2">[<a href="#top">Top</a>]</font>
                            <p/>
                        </tr>
                    </xsl:if>

                    <!-- FLIGHT CONTROL -->
                    <xsl:if test="flight_control">
                        <a name="flightcontrol"/>
                        <tr bgcolor="FFDDDD">
                            <table width="100%" style="font-family:Arial; font-size:90%"
                                bgcolor="FFDDDD" cellpadding="0" cellspacing="0">
                                <tr>
                                    <td colspan="2">
                                        <b>FLIGHT CONTROL</b>
                                    </td>
                                </tr>
                                <tr>
                                    <td colspan="2" height="3">
                                        <hr size="1"/>
                                    </td>
                                </tr>
                                <xsl:for-each select="flight_control/channel">
                                    <tr>
                                        <td valign="top">
                                            <font face="Arial" size="2">
                                                <b>Channel </b>
                                                <xsl:value-of select="@name"/>
                                            </font>
                                        </td>
                                        <xsl:variable name="ucletters">ABCDEFGHIJKLMNOPQRSTUVWXYZ </xsl:variable>
                                        <xsl:variable name="lcletters">abcdefghijklmnopqrstuvwxyz-</xsl:variable>

                                        <td>
                                            <xsl:for-each select="child::*">
                                                <font face="Arial" size="2">
                                                  <p><b>Component: </b><xsl:value-of
                                                  select="@name"/>, Type:
                                                  <xsl:choose>
                                                      <!-- LAG FILTER -->
                                                  <xsl:when test="name() = 'lag_filter'">
                                                  <i>Lag filter</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  <table>
                                                  <tr>
                                                  <td nowrap="1"
                                                  valign="center"
                                                  align="left">
                                                  <font face="Arial"
                                                  size="2">
                                                  Filter:</font>
                                                  </td>
                                                  <td align="center"
                                                  nowrap="1">
                                                  <font face="Arial"
                                                  size="2">
                                                  <xsl:value-of
                                                  select="c1"/><hr
                                                  width="100%"/> s
                                                  + <xsl:value-of
                                                  select="c1"/>
                                                  </font>
                                                  </td>
                                                  </tr>
                                                  </table>
                                                  </xsl:when>
                                                      <!-- INTEGRATOR -->
                                                  <xsl:when test="name() = 'integrator'">
                                                  <i>Integrator</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  <table>
                                                  <tr>
                                                  <td nowrap="1"
                                                  valign="center"
                                                  align="left">
                                                  <font face="Arial"
                                                  size="2">
                                                  Filter:</font>
                                                  </td>
                                                  <td align="center"
                                                  nowrap="1">
                                                  <font face="Arial"
                                                  size="2">
                                                  <xsl:value-of
                                                  select="c1"/><hr
                                                  width="100%"/>
                                                  s</font>
                                                  </td>
                                                  </tr>
                                                  </table>
                                                  <xsl:if test="trigger"> Trigger
                                                  property: <xsl:value-of
                                                  select="trigger"/>
                                                  </xsl:if>
                                                  </xsl:when>
                                                      <!-- LEAD LAG FILTER -->
                                                  <xsl:when
                                                  test="name() =  'lead_lag_filter'">
                                                  <i>Lead-lag filter</i><br/>
                                                  Component property name:
                                                  <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  <table>
                                                  <tr>
                                                  <td nowrap="1"
                                                  valign="center"
                                                  align="left">
                                                  <font face="Arial"
                                                  size="2"
                                                  >Filter:</font>
                                                  </td>
                                                  <td align="center"
                                                  nowrap="1">
                                                  <font face="Arial"
                                                  size="2">
                                                  <xsl:if
                                                  test="count(c1)>0"
                                                  ><xsl:value-of
                                                  select="c1"
                                                  />s</xsl:if>
                                                  <xsl:if
                                                  test="count(c1)=1 and count(c2)=1"
                                                  > + </xsl:if>
                                                  <xsl:if
                                                  test="count(c2)>0">
                                                  <xsl:value-of
                                                  select="c2"
                                                  />
                                                  </xsl:if>
                                                  <hr width="100%"/>
                                                  <xsl:if
                                                  test="count(c3)>0"
                                                  ><xsl:value-of
                                                  select="c3"
                                                  />s</xsl:if>
                                                  <xsl:if
                                                  test="count(c3)=1 and count(c4)=1"
                                                  > + </xsl:if>
                                                  <xsl:if
                                                  test="count(c4)>0">
                                                  <xsl:value-of
                                                  select="c4"
                                                  />
                                                  </xsl:if>
                                                  </font>
                                                  </td>
                                                  </tr>
                                                  </table>
                                                  </xsl:when>
                                                      <!-- SECOND ORDER FILTER -->
                                                      <xsl:when
                                                  test="name() =  'second_order_filter'">
                                                  <i>Second order filter</i><br/>
                                                  Component property name:
                                                  <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  <table>
                                                  <tr>
                                                  <td nowrap="1"
                                                  valign="center"
                                                  align="left">
                                                  <font face="Arial"
                                                  size="2"
                                                  >Filter:</font>
                                                  </td>
                                                  <td align="center"
                                                  nowrap="1">
                                                  <font face="Arial"
                                                  size="2">
                                                  <xsl:value-of
                                                  select="c1"
                                                  />s<sup>2</sup>
                                                  + <xsl:value-of
                                                  select="c2"/>s +
                                                  <xsl:value-of
                                                  select="c3"/><hr
                                                  width="100%"/>
                                                  <xsl:value-of
                                                  select="c4"
                                                  />s<sup>2</sup>
                                                  + <xsl:value-of
                                                  select="c5"/>s +
                                                  <xsl:value-of
                                                  select="c6"/>
                                                  </font>
                                                  </td>
                                                  </tr>
                                                  </table>
                                                      </xsl:when>
                                                      <!-- WASHOUT FILTER -->
                                                  <xsl:when
                                                  test="name() = 'washout_filter'">
                                                  <i>Washout filter</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  <table>
                                                  <tr>
                                                  <td nowrap="1"
                                                  valign="center"
                                                  align="left">
                                                  <font face="Arial"
                                                  size="2">
                                                  Filter:</font>
                                                  </td>
                                                  <td align="center"
                                                  nowrap="1">
                                                  <font face="Arial"
                                                  size="2"> s<hr
                                                  width="100%"/> s
                                                  + <xsl:value-of
                                                  select="c1"/>
                                                  </font>
                                                  </td>
                                                  </tr>
                                                  </table>
                                                  </xsl:when>
                                                      <!-- PURE GAIN -->
                                                  <xsl:when test="name() =  'pure_gain'">
                                                  <i>Gain</i><br/> Component property
                                                  name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  </xsl:when>
                                                      <!-- SCHEDULED GAIN -->
                                                  <xsl:when
                                                  test="name() =  'scheduled_gain'">
                                                  <i>Scheduled gain</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  </xsl:when>
                                                      <!-- AEROSURFACE SCALE -->
                                                  <xsl:when
                                                  test="name() =  'aerosurface_scale'">
                                                  <i>Aerosurface scale</i><br/>
                                                  Component property name:
                                                  <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  </xsl:when>
                                                      <!-- SUMMER -->
                                                  <xsl:when test="name() =  'summer'">
                                                  <i>Summer</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:for-each select="input"> Input: <i>
                                                  <xsl:value-of select="."/>
                                                  </i><br/>
                                                  </xsl:for-each>
                                                  </xsl:when>
                                                      <!-- DEADBAND -->
                                                  <xsl:when test="name() =  'deadband'">
                                                  <i>Deadband</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  </xsl:when>
                                                      <!-- SWITCH -->
                                                  <xsl:when test="name() =  'switch'">
                                                  <i>Switch</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  <ul>
                                                  <xsl:for-each select="test">
                                                  <li>
                                                  <xsl:choose>
                                                  <xsl:when
                                                  test="@logic='AND'"
                                                  > If all of the
                                                  following tests
                                                  are true: </xsl:when>
                                                  <xsl:when
                                                  test="@logic='OR'"
                                                  > If any of the
                                                  following tests
                                                  are true: </xsl:when>
                                                  <xsl:when
                                                  test="count(@logic)=0"
                                                  > If all of the
                                                  following tests
                                                  are true:
                                                  </xsl:when>
                                                  </xsl:choose>
                                                  <pre style="font-size:90%"><xsl:value-of select="."/></pre>
                                                  then the switch value
                                                  is: <xsl:value-of
                                                  select="@value"
                                                  /><br/></li>
                                                  </xsl:for-each>
                                                  <xsl:if test="default/@value">
                                                  <li>Otherwise, the value is:
                                                  <xsl:value-of
                                                  select="default/@value"
                                                  /></li>
                                                  </xsl:if>
                                                  <br/>
                                                  </ul>
                                                  </xsl:when>
                                                      <!-- KINEMATIC -->
                                                  <xsl:when test="name() =  'kinematic'">
                                                  <i>Kinematic</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  </xsl:when>
                                                      <!-- FCS FUNCTION -->
                                                  <xsl:when
                                                  test="name() =  'fcs_function'">
                                                  <i>Function</i><br/> Component
                                                  property name: <i>fcs/<xsl:value-of
                                                  select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                  /></i><br/>
                                                  <xsl:if test="input">Input: <i>
                                                  <xsl:value-of select="input"
                                                  />
                                                  </i><br/></xsl:if>
                                                  <xsl:call-template
                                                  name="process_function"/>
                                                  </xsl:when>
                                                      <!-- PID controller -->
                                                      <xsl:when test="name() = 'pid'">
                                                          <i>PID Controller</i><br/> Component
                                                          property name: <i>fcs/<xsl:value-of
                                                              select="translate(normalize-space(@name), $ucletters, $lcletters)"
                                                          /></i><br/>
                                                          <xsl:if test="input">Input: <i>
                                                              <xsl:value-of select="input"/>
                                                          </i><br/></xsl:if>
                                                          <table>
                                                              <tr>
                                                                  <td nowrap="1" valign="center" align="left">
                                                                      <font face="Arial" size="2">Proportional gain (kp): <xsl:value-of select="kp"/></font><br/>
                                                                      <font face="Arial" size="2">Integral gain (ki): <xsl:value-of select="ki"/></font><br/>
                                                                      <font face="Arial" size="2">Derivative gain (kd): <xsl:value-of select="kd"/></font>
                                                                  </td>
                                                              </tr>
                                                          </table>
                                                          <xsl:if test="trigger"> Trigger
                                                              property: <xsl:value-of
                                                                  select="trigger"/>
                                                          </xsl:if>
                                                      </xsl:when>
                                                  </xsl:choose>
                                                  <xsl:if test="clipto">
                                                  <xsl:if test="clipto/min">Minimum limit:
                                                  <xsl:value-of
                                                  select="clipto/min"/><br/></xsl:if>
                                                  <xsl:if test="clipto/max">Maximum limit:
                                                  <xsl:value-of
                                                  select="clipto/max"
                                                  /><br/></xsl:if>
                                                  </xsl:if>
                                                  <xsl:if test="output">Output to: <i>
                                                  <xsl:value-of select="output"/>
                                                  </i></xsl:if>
                                                  </p>
                                                </font>
                                            </xsl:for-each>
                                            <p/>
                                        </td>
                                    </tr>
                                </xsl:for-each>
                            </table>
                        </tr>
                        <tr>
                            <br/>
                            <font face="Arial" size="2">[<a href="#top">Top</a>]</font>
                            <p/>
                        </tr>
                    </xsl:if>

                    <!-- AERODYNAMICS -->
                    <xsl:if test="aerodynamics">
                        <a name="aerodynamics"/>
                        <tr bgcolor="CCCCCC">
                            <table width="100%" style="font-family:Arial; font-size:90%"
                                bgcolor="EEEEEE" cellpadding="0" cellspacing="0">
                                <tr>
                                    <td colspan="3">
                                        <b>AERODYNAMICS</b>
                                    </td>
                                </tr>
                                <tr>
                                    <td colspan="3" height="3">
                                        <hr size="1"/>
                                    </td>
                                </tr>
                                <tr>
                                    <td colspan="3">The aerodynamics specification for this aircraft
                                        consists of <xsl:value-of
                                            select="count(aerodynamics/function)"/> function[s] and
                                        force components in <xsl:value-of
                                            select="count(aerodynamics/axis)"/> axes.<p/></td>
                                </tr>
                                <xsl:if test="count(aerodynamics/function) >0">
                                    <tr>
                                        <td colspan="3">
                                            <b>Global Functions</b>
                                        </td>
                                    </tr>
                                    <xsl:for-each select="aerodynamics/function">
                                        <tr>
                                            <td width="300" valign="top">
                                                <xsl:value-of select="@name"/>
                                            </td>
                                            <td colspan="2">
                                                <xsl:value-of select="description"/>
                                                <p/>
                                                <xsl:call-template name="process_function"/>
                                            </td>
                                        </tr>
                                    </xsl:for-each>
                                </xsl:if>
                                <xsl:for-each select="aerodynamics/axis">
                                    <tr>
                                        <td colspan="3">
                                            <b><xsl:value-of select="@name"/> Axis</b>
                                        </td>
                                    </tr>
                                    <xsl:for-each select="function">
                                        <tr>
                                            <td width="300" valign="top">
                                                <xsl:value-of select="@name"/>
                                            </td>
                                            <td colspan="2">
                                                <xsl:value-of select="description"/>
                                                <p/>
                                                <xsl:call-template name="process_function"/>
                                            </td>
                                        </tr>
                                    </xsl:for-each>
                                </xsl:for-each>
                            </table>
                        </tr>

                        <tr>
                            <br/>
                            <font face="Arial" size="2">[<a href="#top">Top</a>]</font>
                            <p/>
                        </tr>

                    </xsl:if>
                    <xsl:if test="input">

                        <!-- INPUT -->
                        <a name="input"/>
                        <tr bgcolor="BBBBBB">
                            <table width="100%" style="font-family:Arial; font-size:90%"
                                bgcolor="EEEEEE" cellpadding="0" cellspacing="0">
                                <tr>
                                    <td colspan="2">
                                        <b>INPUT</b>
                                    </td>
                                </tr>
                                <tr>
                                    <td colspan="2" height="3">
                                        <hr size="1"/>
                                    </td>
                                </tr>
                                <tr>
                                    <td>Input accepted on port <xsl:value-of select="input/@port"
                                    />.</td>
                                </tr>
                            </table>
                        </tr>

                        <tr>
                            <br/>
                            <font face="Arial" size="2">[<a href="#top">Top</a>]</font>
                            <p/>
                        </tr>

                    </xsl:if>
                    <xsl:if test="output">

                        <!-- OUTPUT -->
                        <a name="output"/>
                        <tr bgcolor="BBBBBB">
                            <table width="100%" style="font-family:Arial; font-size:90%"
                                bgcolor="EEEEEE" cellpadding="0" cellspacing="0">
                                <tr>
                                    <td colspan="2">
                                        <b>OUTPUT</b>
                                    </td>
                                </tr>
                                <tr>
                                    <td colspan="2" height="3">
                                        <hr size="1"/>
                                    </td>
                                </tr>
                            </table>
                        </tr>

                        <tr>
                            <br/>
                            <font face="Arial" size="2">[<a href="#top">Top</a>]</font>
                            <p/>
                        </tr>
                    </xsl:if>

                </table>

            </body>
        </html>
    </xsl:template>

    <!-- Process Functions -->

    <xsl:template name="process_function">
        <xsl:for-each select="product">
            <xsl:call-template name="process_products"/>
        </xsl:for-each>
    </xsl:template>

    <!-- Process products/tables, etc. -->

    <xsl:template name="process_products"> [ <xsl:for-each select="property">
            <xsl:call-template name="parse_property">
                <xsl:with-param name="property_string" select="."/>
            </xsl:call-template>
        </xsl:for-each>
        <xsl:for-each select="value"> (<xsl:value-of select="."/>) </xsl:for-each>
        <xsl:for-each select="table">
            <xsl:choose>
                <xsl:when test="@name">(<xsl:value-of select="@name"/>) </xsl:when>
                <xsl:otherwise>(table) </xsl:otherwise>
            </xsl:choose>
        </xsl:for-each> ] </xsl:template>

    <xsl:template name="parse_property">
        <xsl:param name="property_string"/>
        <xsl:choose>
            <xsl:when test="contains($property_string,'/')=0">
                <xsl:choose>
                    <xsl:when test="$property_string = 'beta-rad'"> (&#x3B2;<sub>rad</sub>) </xsl:when>
                    <xsl:when test="$property_string = 'alpha-rad'"> (&#x3B1;<sub>rad</sub>) </xsl:when>
                    <xsl:when test="$property_string = 'Sw-sqft'"> (S<sub>w</sub>) </xsl:when>
                    <xsl:when test="$property_string = 'bw-ft'"> (b<sub>w</sub>) </xsl:when>
                    <xsl:when test="$property_string = 'qbar-psf'"> (qbar) </xsl:when>
                    <xsl:when test="$property_string = 'cl-squared-norm'">
                        (C<sub>L</sub><sup>2</sup>) </xsl:when>
                    <xsl:when test="$property_string = 'mach-norm'"> (M) </xsl:when>
                    <xsl:when test="$property_string = 'bi2vel'"> [b<sub>w</sub>/(2V)] </xsl:when>
                    <xsl:otherwise> (<xsl:value-of select="$property_string"/>) </xsl:otherwise>
                </xsl:choose>
            </xsl:when>
            <xsl:otherwise>
                <xsl:call-template name="parse_property">
                    <xsl:with-param name="property_string"
                        select="substring-after($property_string,'/')"/>
                </xsl:call-template>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>

</xsl:stylesheet>
