<?xml version="1.0"?>
<?xml-stylesheet type="text/xsl" href="http://jsbsim.sf.net/JSBSimScript.xsl"?>
<runscript xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    xsi:noNamespaceSchemaLocation="http://jsbsim.sf.net/JSBSimScript.xsd"
    name="F-16 aircraft test run">
  <description>This run is for testing an F-16</description>
  <use aircraft="f16" initialize="reset00"/>
  <run start="0.0" end="700" dt="0.00833333">
    <event name="starter">
      <notify/>
      <condition>
        simulation/sim-time-sec >= 0.25
      </condition>
      <set name="propulsion/starter_cmd" value="1"/>
    </event>
    <event>
      <notify/>
      <condition>
        propulsion/engine[0]/n2 >= 15
      </condition>
      <set name="propulsion/cutoff_cmd" value="0"/>
      <set name="fcs/throttle-cmd-norm[0]" value="0.5" action="FG_RAMP" tc ="2"/>
    </event>
    <event>
      <notify/>
      <condition>
        propulsion/engine[0]/n2 >= 50
      </condition>
      <set name="fcs/throttle-cmd-norm[0]" value="1.0" action="FG_RAMP" tc ="2"/>
    </event>
    <event name="15-second control surface setting">
      <condition> simulation/sim-time-sec >= 15.00 </condition>
      <set name="fcs/elevator-cmd-norm" value="-0.05"/>
      <notify/>
    </event>
  </run>
</runscript>
