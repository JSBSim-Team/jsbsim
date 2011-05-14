<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
    <xsl:output method="html" encoding="UTF-8"/>
    <xsl:template match="/runscript">
        <html>
            <head>
                <title><xsl:value-of select="@name"/></title>
            </head>
            <body style="font-family:Arial;font-size:90%">
                <font face="Arial" size="3" color="224488"><b>JSBSim Script:<xsl:value-of select="@name"/></b></font><br/>
                <font face="Arial" size="2"><b>Description</b>: <xsl:value-of select="description"/></font><br/>
                <font face="Arial" size="2"><b>Aircraft</b>: <xsl:value-of select="use/@aircraft"/></font><br/>
                <font face="Arial" size="2"><b>Initial Conditions</b>:<xsl:value-of select="use/@initialize"/></font><br/>
                <font face="Arial" size="2"><b>Starts at</b>:<xsl:value-of select="run/@start"/></font><br/>
                <font face="Arial" size="2"><b>Ends at</b>:<xsl:value-of select="run/@end"/></font><br/>
                <font face="Arial" size="2"><b>Delta time</b>:<xsl:value-of select="run/@dt"/></font><br/>
                    <table width="100%">
                    <font face="Arial" size="2">
		    <xsl:if test="run/property">
                      <tr bgcolor="EEEEEE"><td><hr width="100%"/><font face="Arial" size="2"><font color="#0033ff"><b>Local Properties</b>:</font>
		      <ul>
                        <xsl:for-each select="run/property">
                          <li><xsl:value-of select="."/></li>
                        </xsl:for-each>
                      </ul>
		      </font></td></tr>
		    </xsl:if>
                    <xsl:for-each select="run/event">
                        <tr bgcolor="EEEEEE"><td><hr width="100%"/><font face="Arial" size="2" color="#0033ff"><b>Event</b>: <xsl:value-of select="@name"/></font>
                        <xsl:if test="description"><font face="Arial" size="2"><br/><b>Description</b>: <xsl:value-of select="description"/></font></xsl:if>
                        </td></tr>
                        <tr><td>
                            <font face="Arial" size="2">
                            <xsl:if test="condition">
                              <b>Test Conditions</b>:
                              <ul>
                              <xsl:for-each select="condition">
                                <li><xsl:value-of select="."/></li>
                              </xsl:for-each>
                              </ul>
                            </xsl:if>
                            <xsl:if test="set"> <!-- false if no set actions -->
                            <b>Actions</b>:
                            <xsl:if test="set">
                            <ul>
                              <xsl:for-each select="set">
                                  <li>
                                  <xsl:if test="@type">
                                      Change <xsl:value-of select="@name"/> by <xsl:value-of select="@value"/>
                                  </xsl:if>
                                  <xsl:if test="not(@type)">
                                      Set <xsl:value-of select="@name"/> to <xsl:value-of select="@value"/>
                                  </xsl:if>
                                  <xsl:if test="@action">
                                      <xsl:if test="@action = 'FG_STEP'">
                                          via step 
                                      </xsl:if>
                                      <xsl:if test="@action = 'FG_EXP'">
                                          via exponential input 
                                      </xsl:if>
                                      <xsl:if test="@action = 'FG_RAMP'">
                                          via ramp input 
                                      </xsl:if>
                                      <xsl:if test="@tc">
                                          over <xsl:value-of select="@tc"/> seconds 
                                      </xsl:if>
                                  </xsl:if>
                                  </li>
                              </xsl:for-each>
                            </ul>
                            </xsl:if>
                            
                            </xsl:if> <!-- Actions -->
                            
                            <xsl:if test="notify">
                                When this event is triggered, a notification message will be shown
                                <xsl:if test="notify/property">
                                    and the values of following property or properties will be displayed:
                                    <ul>
                                    <xsl:for-each select="notify/property">
                                        <li><xsl:value-of select="."/></li>
                                    </xsl:for-each>
                                    </ul>
                                </xsl:if>
                            </xsl:if>
                            </font>
                        </td></tr>
                        <tr><td></td></tr>
                    </xsl:for-each>
                    </font>
                    </table>
            </body>
        </html>
    </xsl:template>
</xsl:stylesheet>
