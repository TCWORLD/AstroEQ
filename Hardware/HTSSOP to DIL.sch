<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE eagle SYSTEM "eagle.dtd">
<eagle version="6.0">
<drawing>
<settings>
<setting alwaysvectorfont="no"/>
<setting verticaltext="up"/>
</settings>
<grid distance="0.1" unitdist="inch" unit="inch" style="lines" multiple="1" display="no" altdistance="0.01" altunitdist="inch" altunit="inch"/>
<layers>
<layer number="1" name="Top" color="4" fill="1" visible="no" active="no"/>
<layer number="2" name="Route2" color="1" fill="3" visible="no" active="no"/>
<layer number="3" name="Route3" color="4" fill="3" visible="no" active="no"/>
<layer number="14" name="Route14" color="1" fill="6" visible="no" active="no"/>
<layer number="15" name="Route15" color="4" fill="6" visible="no" active="no"/>
<layer number="16" name="Bottom" color="1" fill="1" visible="no" active="no"/>
<layer number="17" name="Pads" color="2" fill="1" visible="no" active="no"/>
<layer number="18" name="Vias" color="2" fill="1" visible="no" active="no"/>
<layer number="19" name="Unrouted" color="6" fill="1" visible="no" active="no"/>
<layer number="20" name="Dimension" color="15" fill="1" visible="no" active="no"/>
<layer number="21" name="tPlace" color="7" fill="1" visible="no" active="no"/>
<layer number="22" name="bPlace" color="7" fill="1" visible="no" active="no"/>
<layer number="23" name="tOrigins" color="15" fill="1" visible="no" active="no"/>
<layer number="24" name="bOrigins" color="15" fill="1" visible="no" active="no"/>
<layer number="25" name="tNames" color="7" fill="1" visible="no" active="no"/>
<layer number="26" name="bNames" color="7" fill="1" visible="no" active="no"/>
<layer number="27" name="tValues" color="7" fill="1" visible="no" active="no"/>
<layer number="28" name="bValues" color="7" fill="1" visible="no" active="no"/>
<layer number="29" name="tStop" color="7" fill="3" visible="no" active="no"/>
<layer number="30" name="bStop" color="7" fill="6" visible="no" active="no"/>
<layer number="31" name="tCream" color="7" fill="4" visible="no" active="no"/>
<layer number="32" name="bCream" color="7" fill="5" visible="no" active="no"/>
<layer number="33" name="tFinish" color="6" fill="3" visible="no" active="no"/>
<layer number="34" name="bFinish" color="6" fill="6" visible="no" active="no"/>
<layer number="35" name="tGlue" color="7" fill="4" visible="no" active="no"/>
<layer number="36" name="bGlue" color="7" fill="5" visible="no" active="no"/>
<layer number="37" name="tTest" color="7" fill="1" visible="no" active="no"/>
<layer number="38" name="bTest" color="7" fill="1" visible="no" active="no"/>
<layer number="39" name="tKeepout" color="4" fill="11" visible="no" active="no"/>
<layer number="40" name="bKeepout" color="1" fill="11" visible="no" active="no"/>
<layer number="41" name="tRestrict" color="4" fill="10" visible="no" active="no"/>
<layer number="42" name="bRestrict" color="1" fill="10" visible="no" active="no"/>
<layer number="43" name="vRestrict" color="2" fill="10" visible="no" active="no"/>
<layer number="44" name="Drills" color="7" fill="1" visible="no" active="no"/>
<layer number="45" name="Holes" color="7" fill="1" visible="no" active="no"/>
<layer number="46" name="Milling" color="3" fill="1" visible="no" active="no"/>
<layer number="47" name="Measures" color="7" fill="1" visible="no" active="no"/>
<layer number="48" name="Document" color="7" fill="1" visible="no" active="no"/>
<layer number="49" name="Reference" color="7" fill="1" visible="no" active="no"/>
<layer number="51" name="tDocu" color="6" fill="1" visible="no" active="no"/>
<layer number="52" name="bDocu" color="7" fill="1" visible="no" active="no"/>
<layer number="91" name="Nets" color="2" fill="1" visible="yes" active="yes"/>
<layer number="92" name="Busses" color="1" fill="1" visible="yes" active="yes"/>
<layer number="93" name="Pins" color="2" fill="1" visible="no" active="yes"/>
<layer number="94" name="Symbols" color="4" fill="1" visible="yes" active="yes"/>
<layer number="95" name="Names" color="7" fill="1" visible="yes" active="yes"/>
<layer number="96" name="Values" color="7" fill="1" visible="yes" active="yes"/>
<layer number="97" name="Info" color="7" fill="1" visible="yes" active="yes"/>
<layer number="98" name="Guide" color="6" fill="1" visible="yes" active="yes"/>
</layers>
<schematic xreflabel="%F%N/%S.%C%R" xrefpart="/%S.%C%R">
<libraries>
<library name="texas">
<description>&lt;b&gt;Texas Instruments Devices&lt;/b&gt;&lt;p&gt;
 &lt;author&gt;Created by librarian@cadsoft.de&lt;/author&gt;</description>
<packages>
<package name="HTSSOP28PWP">
<description>&lt;b&gt;TPS40055PWP&lt;/b&gt; (R-PDSO-G**) PowerPAD (TM) 28 pol.&lt;p&gt;
PLASTIC SMALL-OUTLINE PACKAGE&lt;br&gt;
Source: www.ti.com ... TPS40055 package.pdf &amp; slma002.pdf</description>
<wire x1="-4.5" y1="2.15" x2="4.5" y2="2.15" width="0.2032" layer="21"/>
<wire x1="4.5" y1="2.15" x2="4.5" y2="-2.15" width="0.2032" layer="21"/>
<wire x1="4.5" y1="-2.15" x2="-4.5" y2="-2.15" width="0.2032" layer="21"/>
<wire x1="-4.5" y1="-2.15" x2="-4.5" y2="2.15" width="0.2032" layer="21"/>
<circle x="-3.8" y="-1.45" radius="0.3" width="0.2032" layer="21"/>
<smd name="1" x="-4.21" y="-2.9" dx="0.27" dy="1.2" layer="1" stop="no" cream="no"/>
<smd name="2" x="-3.575" y="-2.9" dx="0.27" dy="1.2" layer="1" stop="no" cream="no"/>
<smd name="3" x="-2.925" y="-2.9" dx="0.27" dy="1.2" layer="1" stop="no" cream="no"/>
<smd name="4" x="-2.275" y="-2.9" dx="0.27" dy="1.2" layer="1" stop="no" cream="no"/>
<smd name="5" x="-1.625" y="-2.9" dx="0.27" dy="1.2" layer="1" stop="no" cream="no"/>
<smd name="6" x="-0.975" y="-2.9" dx="0.27" dy="1.2" layer="1" stop="no" cream="no"/>
<smd name="7" x="-0.325" y="-2.9" dx="0.27" dy="1.2" layer="1" stop="no" cream="no"/>
<smd name="8" x="0.325" y="-2.9" dx="0.27" dy="1.2" layer="1" stop="no" cream="no"/>
<smd name="9" x="0.975" y="-2.9" dx="0.27" dy="1.2" layer="1" stop="no" cream="no"/>
<smd name="10" x="1.625" y="-2.9" dx="0.27" dy="1.2" layer="1" stop="no" cream="no"/>
<smd name="11" x="2.275" y="-2.9" dx="0.27" dy="1.2" layer="1" stop="no" cream="no"/>
<smd name="14" x="4.395" y="-2.1325" dx="0.61" dy="2.735" layer="1" stop="no" cream="no"/>
<smd name="15" x="4.21" y="2.9" dx="0.27" dy="1.2" layer="1" rot="R180" stop="no" cream="no"/>
<smd name="28" x="-4.395" y="2.45" dx="0.61" dy="2.1" layer="1" rot="R180" stop="no" cream="no"/>
<smd name="EXP" x="0" y="0.3175" dx="9.4" dy="3" layer="1" stop="no" cream="no"/>
<smd name="12" x="2.925" y="-2.9" dx="0.27" dy="1.2" layer="1" stop="no" cream="no"/>
<smd name="13" x="3.575" y="-2.9" dx="0.27" dy="1.2" layer="1" stop="no" cream="no"/>
<smd name="16" x="3.575" y="2.9" dx="0.27" dy="1.2" layer="1" rot="R180" stop="no" cream="no"/>
<smd name="17" x="2.925" y="2.9" dx="0.27" dy="1.2" layer="1" rot="R180" stop="no" cream="no"/>
<smd name="18" x="2.275" y="2.9" dx="0.27" dy="1.2" layer="1" rot="R180" stop="no" cream="no"/>
<smd name="19" x="1.625" y="2.9" dx="0.27" dy="1.2" layer="1" rot="R180" stop="no" cream="no"/>
<smd name="20" x="0.975" y="2.9" dx="0.27" dy="1.2" layer="1" rot="R180" stop="no" cream="no"/>
<smd name="21" x="0.325" y="2.9" dx="0.27" dy="1.2" layer="1" rot="R180" stop="no" cream="no"/>
<smd name="22" x="-0.325" y="2.9" dx="0.27" dy="1.2" layer="1" rot="R180" stop="no" cream="no"/>
<smd name="23" x="-0.975" y="2.9" dx="0.27" dy="1.2" layer="1" rot="R180" stop="no" cream="no"/>
<smd name="24" x="-1.625" y="2.9" dx="0.27" dy="1.2" layer="1" rot="R180" stop="no" cream="no"/>
<smd name="25" x="-2.275" y="2.9" dx="0.27" dy="1.2" layer="1" rot="R180" stop="no" cream="no"/>
<smd name="26" x="-2.925" y="2.9" dx="0.27" dy="1.2" layer="1" rot="R180" stop="no" cream="no"/>
<smd name="27" x="-3.575" y="2.9" dx="0.27" dy="1.2" layer="1" rot="R180" stop="no" cream="no"/>
<text x="-4.46" y="3.81" size="1.27" layer="25">&gt;NAME</text>
<text x="-4.46" y="-5.08" size="1.27" layer="27">&gt;VALUE</text>
<rectangle x1="-4.375" y1="-3.3" x2="-4.075" y2="-2.2" layer="51"/>
<rectangle x1="-3.725" y1="-3.3" x2="-3.425" y2="-2.2" layer="51"/>
<rectangle x1="-3.075" y1="-3.3" x2="-2.775" y2="-2.2" layer="51"/>
<rectangle x1="-2.425" y1="-3.3" x2="-2.125" y2="-2.2" layer="51"/>
<rectangle x1="-1.775" y1="-3.3" x2="-1.475" y2="-2.2" layer="51"/>
<rectangle x1="-1.125" y1="-3.3" x2="-0.825" y2="-2.2" layer="51"/>
<rectangle x1="-0.475" y1="-3.3" x2="-0.175" y2="-2.2" layer="51"/>
<rectangle x1="0.175" y1="-3.3" x2="0.475" y2="-2.2" layer="51"/>
<rectangle x1="0.825" y1="-3.3" x2="1.125" y2="-2.2" layer="51"/>
<rectangle x1="1.475" y1="-3.3" x2="1.775" y2="-2.2" layer="51"/>
<rectangle x1="2.125" y1="-3.3" x2="2.425" y2="-2.2" layer="51"/>
<rectangle x1="4.075" y1="-3.3" x2="4.375" y2="-2.2" layer="51"/>
<rectangle x1="4.075" y1="2.2" x2="4.375" y2="3.3" layer="51" rot="R180"/>
<rectangle x1="-4.375" y1="2.2" x2="-4.075" y2="3.3" layer="51" rot="R180"/>
<rectangle x1="-4.36" y1="-3.5" x2="-4.09" y2="-2.3" layer="29"/>
<rectangle x1="-3.71" y1="-3.5" x2="-3.44" y2="-2.3" layer="29"/>
<rectangle x1="-3.06" y1="-3.5" x2="-2.79" y2="-2.3" layer="29"/>
<rectangle x1="-2.41" y1="-3.5" x2="-2.14" y2="-2.3" layer="29"/>
<rectangle x1="-1.76" y1="-3.5" x2="-1.49" y2="-2.3" layer="29"/>
<rectangle x1="-1.11" y1="-3.5" x2="-0.84" y2="-2.3" layer="29"/>
<rectangle x1="-0.46" y1="-3.5" x2="-0.19" y2="-2.3" layer="29"/>
<rectangle x1="0.19" y1="-3.5" x2="0.46" y2="-2.3" layer="29"/>
<rectangle x1="0.84" y1="-3.5" x2="1.11" y2="-2.3" layer="29"/>
<rectangle x1="1.49" y1="-3.5" x2="1.76" y2="-2.3" layer="29"/>
<rectangle x1="2.14" y1="-3.5" x2="2.41" y2="-2.3" layer="29"/>
<rectangle x1="4.09" y1="-3.5" x2="4.36" y2="-2.3" layer="29"/>
<rectangle x1="4.09" y1="2.3" x2="4.36" y2="3.5" layer="29" rot="R180"/>
<rectangle x1="2.14" y1="2.3" x2="2.41" y2="3.5" layer="29" rot="R180"/>
<rectangle x1="1.49" y1="2.3" x2="1.76" y2="3.5" layer="29" rot="R180"/>
<rectangle x1="0.84" y1="2.3" x2="1.11" y2="3.5" layer="29" rot="R180"/>
<rectangle x1="0.19" y1="2.3" x2="0.46" y2="3.5" layer="29" rot="R180"/>
<rectangle x1="-0.46" y1="2.3" x2="-0.19" y2="3.5" layer="29" rot="R180"/>
<rectangle x1="-1.11" y1="2.3" x2="-0.84" y2="3.5" layer="29" rot="R180"/>
<rectangle x1="-1.76" y1="2.3" x2="-1.49" y2="3.5" layer="29" rot="R180"/>
<rectangle x1="-2.41" y1="2.3" x2="-2.14" y2="3.5" layer="29" rot="R180"/>
<rectangle x1="-3.06" y1="2.3" x2="-2.79" y2="3.5" layer="29" rot="R180"/>
<rectangle x1="-3.71" y1="2.3" x2="-3.44" y2="3.5" layer="29" rot="R180"/>
<rectangle x1="-4.36" y1="2.3" x2="-4.09" y2="3.5" layer="29" rot="R180"/>
<rectangle x1="-3.19" y1="-1.1" x2="3.19" y2="1.1" layer="29"/>
<rectangle x1="2.79" y1="2.3" x2="3.06" y2="3.5" layer="29" rot="R180"/>
<rectangle x1="2.79" y1="-3.5" x2="3.06" y2="-2.3" layer="29"/>
<rectangle x1="3.44" y1="2.3" x2="3.71" y2="3.5" layer="29" rot="R180"/>
<rectangle x1="3.44" y1="-3.5" x2="3.71" y2="-2.3" layer="29"/>
<rectangle x1="2.775" y1="-3.3" x2="3.075" y2="-2.2" layer="51"/>
<rectangle x1="3.425" y1="-3.3" x2="3.725" y2="-2.2" layer="51"/>
<rectangle x1="3.425" y1="2.2" x2="3.725" y2="3.3" layer="51" rot="R180"/>
<rectangle x1="2.775" y1="2.2" x2="3.075" y2="3.3" layer="51" rot="R180"/>
<rectangle x1="2.125" y1="2.2" x2="2.425" y2="3.3" layer="51" rot="R180"/>
<rectangle x1="1.475" y1="2.2" x2="1.775" y2="3.3" layer="51" rot="R180"/>
<rectangle x1="0.825" y1="2.2" x2="1.125" y2="3.3" layer="51" rot="R180"/>
<rectangle x1="0.175" y1="2.2" x2="0.475" y2="3.3" layer="51" rot="R180"/>
<rectangle x1="-0.475" y1="2.2" x2="-0.175" y2="3.3" layer="51" rot="R180"/>
<rectangle x1="-1.125" y1="2.2" x2="-0.825" y2="3.3" layer="51" rot="R180"/>
<rectangle x1="-1.775" y1="2.2" x2="-1.475" y2="3.3" layer="51" rot="R180"/>
<rectangle x1="-2.425" y1="2.2" x2="-2.125" y2="3.3" layer="51" rot="R180"/>
<rectangle x1="-3.075" y1="2.2" x2="-2.775" y2="3.3" layer="51" rot="R180"/>
<rectangle x1="-3.725" y1="2.2" x2="-3.425" y2="3.3" layer="51" rot="R180"/>
<rectangle x1="-4.35" y1="-3.48" x2="-4.1" y2="-2.31" layer="31"/>
<rectangle x1="-3.7" y1="-3.48" x2="-3.45" y2="-2.31" layer="31"/>
<rectangle x1="-3.05" y1="-3.48" x2="-2.8" y2="-2.31" layer="31"/>
<rectangle x1="-2.4" y1="-3.48" x2="-2.15" y2="-2.31" layer="31"/>
<rectangle x1="-1.75" y1="-3.48" x2="-1.5" y2="-2.31" layer="31"/>
<rectangle x1="-1.1" y1="-3.48" x2="-0.85" y2="-2.31" layer="31"/>
<rectangle x1="-0.45" y1="-3.48" x2="-0.2" y2="-2.31" layer="31"/>
<rectangle x1="0.2" y1="-3.48" x2="0.45" y2="-2.31" layer="31"/>
<rectangle x1="0.85" y1="-3.48" x2="1.1" y2="-2.31" layer="31"/>
<rectangle x1="1.5" y1="-3.48" x2="1.75" y2="-2.31" layer="31"/>
<rectangle x1="2.15" y1="-3.48" x2="2.4" y2="-2.31" layer="31"/>
<rectangle x1="2.8" y1="-3.48" x2="3.05" y2="-2.31" layer="31"/>
<rectangle x1="3.45" y1="-3.48" x2="3.7" y2="-2.31" layer="31"/>
<rectangle x1="4.1" y1="-3.48" x2="4.35" y2="-2.31" layer="31"/>
<rectangle x1="4.1" y1="2.31" x2="4.35" y2="3.48" layer="31" rot="R180"/>
<rectangle x1="3.45" y1="2.31" x2="3.7" y2="3.48" layer="31" rot="R180"/>
<rectangle x1="2.8" y1="2.31" x2="3.05" y2="3.48" layer="31" rot="R180"/>
<rectangle x1="2.15" y1="2.31" x2="2.4" y2="3.48" layer="31" rot="R180"/>
<rectangle x1="1.5" y1="2.31" x2="1.75" y2="3.48" layer="31" rot="R180"/>
<rectangle x1="0.85" y1="2.31" x2="1.1" y2="3.48" layer="31" rot="R180"/>
<rectangle x1="0.2" y1="2.31" x2="0.45" y2="3.48" layer="31" rot="R180"/>
<rectangle x1="-0.45" y1="2.31" x2="-0.2" y2="3.48" layer="31" rot="R180"/>
<rectangle x1="-1.1" y1="2.31" x2="-0.85" y2="3.48" layer="31" rot="R180"/>
<rectangle x1="-1.75" y1="2.31" x2="-1.5" y2="3.48" layer="31" rot="R180"/>
<rectangle x1="-2.4" y1="2.31" x2="-2.15" y2="3.48" layer="31" rot="R180"/>
<rectangle x1="-3.05" y1="2.31" x2="-2.8" y2="3.48" layer="31" rot="R180"/>
<rectangle x1="-3.7" y1="2.31" x2="-3.45" y2="3.48" layer="31" rot="R180"/>
<rectangle x1="-4.35" y1="2.31" x2="-4.1" y2="3.48" layer="31" rot="R180"/>
<rectangle x1="-3.1" y1="-1" x2="3.1" y2="1" layer="31"/>
</package>
</packages>
<symbols>
<symbol name="DRV8824">
<wire x1="-10.16" y1="20.32" x2="10.16" y2="20.32" width="0.254" layer="94"/>
<wire x1="10.16" y1="20.32" x2="10.16" y2="-17.78" width="0.254" layer="94"/>
<wire x1="10.16" y1="-17.78" x2="-10.16" y2="-17.78" width="0.254" layer="94"/>
<wire x1="-10.16" y1="-17.78" x2="-10.16" y2="20.32" width="0.254" layer="94"/>
<text x="-10.16" y="21.59" size="1.778" layer="95">&gt;NAME</text>
<text x="-10.16" y="-20.32" size="1.778" layer="96">&gt;VALUE</text>
<pin name="CP1" x="-12.7" y="17.78" length="short" direction="pas" swaplevel="1"/>
<pin name="CP2" x="-12.7" y="15.24" length="short" direction="pas" swaplevel="1"/>
<pin name="VCP" x="-12.7" y="12.7" length="short" direction="pwr"/>
<pin name="VMA" x="-12.7" y="10.16" length="short" direction="pwr"/>
<pin name="AOUT1" x="-12.7" y="7.62" length="short" direction="out"/>
<pin name="ISENA" x="-12.7" y="5.08" length="short"/>
<pin name="AOUT2" x="-12.7" y="2.54" length="short" direction="out"/>
<pin name="BOUT2" x="-12.7" y="0" length="short" direction="out"/>
<pin name="ISENB" x="-12.7" y="-2.54" length="short"/>
<pin name="BOUT1" x="-12.7" y="-5.08" length="short" direction="out"/>
<pin name="VMB" x="-12.7" y="-7.62" length="short" direction="pwr"/>
<pin name="AVREF" x="-12.7" y="-10.16" length="short" direction="in"/>
<pin name="BVREF" x="-12.7" y="-12.7" length="short" direction="in"/>
<pin name="GND1" x="-12.7" y="-15.24" length="short" direction="pwr"/>
<pin name="V3P3" x="12.7" y="-15.24" length="short" direction="out" rot="R180"/>
<pin name="!RES" x="12.7" y="-12.7" length="short" direction="in" rot="R180"/>
<pin name="!SLP" x="12.7" y="-10.16" length="short" direction="in" rot="R180"/>
<pin name="!ERR" x="12.7" y="-7.62" length="short" direction="out" rot="R180"/>
<pin name="DECAY" x="12.7" y="-5.08" length="short" direction="in" rot="R180"/>
<pin name="DIR" x="12.7" y="-2.54" length="short" direction="in" rot="R180"/>
<pin name="!EN" x="12.7" y="0" length="short" direction="in" rot="R180"/>
<pin name="STEP" x="12.7" y="2.54" length="short" direction="in" rot="R180"/>
<pin name="N/C" x="12.7" y="5.08" length="short" direction="nc" rot="R180"/>
<pin name="MODE0" x="12.7" y="7.62" length="short" direction="in" rot="R180"/>
<pin name="MODE1" x="12.7" y="10.16" length="short" direction="in" rot="R180"/>
<pin name="MODE2" x="12.7" y="12.7" length="short" direction="in" rot="R180"/>
<pin name="!HOME" x="12.7" y="15.24" length="short" direction="out" rot="R180"/>
<pin name="GND2" x="12.7" y="17.78" length="short" direction="pwr" rot="R180"/>
<pin name="PAD" x="0" y="22.86" length="short" direction="pwr" rot="R270"/>
</symbol>
</symbols>
<devicesets>
<deviceset name="DRV8824" prefix="M">
<gates>
<gate name="M1" symbol="DRV8824" x="0" y="0"/>
</gates>
<devices>
<device name="PWP" package="HTSSOP28PWP">
<connects>
<connect gate="M1" pin="!EN" pad="21"/>
<connect gate="M1" pin="!ERR" pad="18"/>
<connect gate="M1" pin="!HOME" pad="27"/>
<connect gate="M1" pin="!RES" pad="16"/>
<connect gate="M1" pin="!SLP" pad="17"/>
<connect gate="M1" pin="AOUT1" pad="5"/>
<connect gate="M1" pin="AOUT2" pad="7"/>
<connect gate="M1" pin="AVREF" pad="12"/>
<connect gate="M1" pin="BOUT1" pad="10"/>
<connect gate="M1" pin="BOUT2" pad="8"/>
<connect gate="M1" pin="BVREF" pad="13"/>
<connect gate="M1" pin="CP1" pad="1"/>
<connect gate="M1" pin="CP2" pad="2"/>
<connect gate="M1" pin="DECAY" pad="19"/>
<connect gate="M1" pin="DIR" pad="20"/>
<connect gate="M1" pin="GND1" pad="14"/>
<connect gate="M1" pin="GND2" pad="28"/>
<connect gate="M1" pin="ISENA" pad="6"/>
<connect gate="M1" pin="ISENB" pad="9"/>
<connect gate="M1" pin="MODE0" pad="24"/>
<connect gate="M1" pin="MODE1" pad="25"/>
<connect gate="M1" pin="MODE2" pad="26"/>
<connect gate="M1" pin="N/C" pad="23"/>
<connect gate="M1" pin="PAD" pad="EXP"/>
<connect gate="M1" pin="STEP" pad="22"/>
<connect gate="M1" pin="V3P3" pad="15"/>
<connect gate="M1" pin="VCP" pad="3"/>
<connect gate="M1" pin="VMA" pad="4"/>
<connect gate="M1" pin="VMB" pad="11"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
</devicesets>
</library>
<library name="ic-package">
<description>&lt;b&gt;IC Packages an Sockets&lt;/b&gt;&lt;p&gt;
&lt;author&gt;Created by librarian@cadsoft.de&lt;/author&gt;</description>
<packages>
<package name="DIL28-6">
<description>&lt;b&gt;Dual In Line Package&lt;/b&gt; 0.6 inch</description>
<wire x1="-17.653" y1="-1.27" x2="-17.653" y2="-6.604" width="0.1524" layer="21"/>
<wire x1="-17.653" y1="1.27" x2="-17.653" y2="-1.27" width="0.1524" layer="21" curve="-180"/>
<wire x1="17.653" y1="-6.604" x2="17.653" y2="6.604" width="0.1524" layer="21"/>
<wire x1="-17.653" y1="6.604" x2="-17.653" y2="1.27" width="0.1524" layer="21"/>
<wire x1="-17.653" y1="6.604" x2="17.653" y2="6.604" width="0.1524" layer="21"/>
<wire x1="-17.653" y1="-6.604" x2="17.653" y2="-6.604" width="0.1524" layer="21"/>
<pad name="1" x="-16.51" y="-7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="2" x="-13.97" y="-7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="3" x="-11.43" y="-7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="4" x="-8.89" y="-7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="5" x="-6.35" y="-7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="6" x="-3.81" y="-7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="7" x="-1.27" y="-7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="8" x="1.27" y="-7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="9" x="3.81" y="-7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="10" x="6.35" y="-7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="11" x="8.89" y="-7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="12" x="11.43" y="-7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="13" x="13.97" y="-7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="14" x="16.51" y="-7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="15" x="16.51" y="7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="16" x="13.97" y="7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="17" x="11.43" y="7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="18" x="8.89" y="7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="19" x="6.35" y="7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="20" x="3.81" y="7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="21" x="1.27" y="7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="22" x="-1.27" y="7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="23" x="-3.81" y="7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="24" x="-6.35" y="7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="25" x="-8.89" y="7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="26" x="-11.43" y="7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="27" x="-13.97" y="7.62" drill="0.8128" shape="long" rot="R90"/>
<pad name="28" x="-16.51" y="7.62" drill="0.8128" shape="long" rot="R90"/>
<text x="-17.78" y="-6.35" size="1.778" layer="25" ratio="10" rot="R90">&gt;NAME</text>
<text x="-14.605" y="-0.9398" size="1.778" layer="27" ratio="10">&gt;VALUE</text>
</package>
<package name="DIL28-3">
<description>&lt;b&gt;Dual In Line Package Small&lt;/b&gt;</description>
<wire x1="17.78" y1="2.921" x2="-17.78" y2="2.921" width="0.1524" layer="21"/>
<wire x1="-17.78" y1="-2.921" x2="17.78" y2="-2.921" width="0.1524" layer="21"/>
<wire x1="17.78" y1="2.921" x2="17.78" y2="-2.921" width="0.1524" layer="21"/>
<wire x1="-17.78" y1="2.921" x2="-17.78" y2="1.016" width="0.1524" layer="21"/>
<wire x1="-17.78" y1="-2.921" x2="-17.78" y2="-1.016" width="0.1524" layer="21"/>
<wire x1="-17.78" y1="1.016" x2="-17.78" y2="-1.016" width="0.1524" layer="21" curve="-180"/>
<pad name="1" x="-16.51" y="-3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="2" x="-13.97" y="-3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="7" x="-1.27" y="-3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="8" x="1.27" y="-3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="3" x="-11.43" y="-3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="4" x="-8.89" y="-3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="6" x="-3.81" y="-3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="5" x="-6.35" y="-3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="9" x="3.81" y="-3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="10" x="6.35" y="-3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="11" x="8.89" y="-3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="12" x="11.43" y="-3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="13" x="13.97" y="-3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="14" x="16.51" y="-3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="15" x="16.51" y="3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="16" x="13.97" y="3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="17" x="11.43" y="3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="18" x="8.89" y="3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="19" x="6.35" y="3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="20" x="3.81" y="3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="21" x="1.27" y="3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="22" x="-1.27" y="3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="23" x="-3.81" y="3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="24" x="-6.35" y="3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="25" x="-8.89" y="3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="26" x="-11.43" y="3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="27" x="-13.97" y="3.81" drill="0.8128" shape="long" rot="R90"/>
<pad name="28" x="-16.51" y="3.81" drill="0.8128" shape="long" rot="R90"/>
<text x="-18.034" y="-2.921" size="1.27" layer="25" ratio="10" rot="R90">&gt;NAME</text>
<text x="-8.509" y="-0.635" size="1.27" layer="27" ratio="10">&gt;VALUE</text>
</package>
<package name="DIL28-4">
<description>&lt;b&gt;Dual In Line Package&lt;/b&gt;</description>
<wire x1="17.78" y1="4.191" x2="-17.78" y2="4.191" width="0.1524" layer="21"/>
<wire x1="-17.78" y1="-4.191" x2="17.78" y2="-4.191" width="0.1524" layer="21"/>
<wire x1="17.78" y1="4.191" x2="17.78" y2="-4.191" width="0.1524" layer="21"/>
<wire x1="-17.78" y1="4.191" x2="-17.78" y2="1.016" width="0.1524" layer="21"/>
<wire x1="-17.78" y1="-4.191" x2="-17.78" y2="-1.016" width="0.1524" layer="21"/>
<wire x1="-17.78" y1="1.016" x2="-17.78" y2="-1.016" width="0.1524" layer="21" curve="-180"/>
<pad name="1" x="-16.51" y="-5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="2" x="-13.97" y="-5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="7" x="-1.27" y="-5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="8" x="1.27" y="-5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="3" x="-11.43" y="-5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="4" x="-8.89" y="-5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="6" x="-3.81" y="-5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="5" x="-6.35" y="-5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="9" x="3.81" y="-5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="10" x="6.35" y="-5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="11" x="8.89" y="-5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="12" x="11.43" y="-5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="13" x="13.97" y="-5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="14" x="16.51" y="-5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="15" x="16.51" y="5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="16" x="13.97" y="5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="17" x="11.43" y="5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="18" x="8.89" y="5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="19" x="6.35" y="5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="20" x="3.81" y="5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="21" x="1.27" y="5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="22" x="-1.27" y="5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="23" x="-3.81" y="5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="24" x="-6.35" y="5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="25" x="-8.89" y="5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="26" x="-11.43" y="5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="27" x="-13.97" y="5.08" drill="0.8128" shape="long" rot="R90"/>
<pad name="28" x="-16.51" y="5.08" drill="0.8128" shape="long" rot="R90"/>
<text x="-18.161" y="-4.191" size="1.778" layer="25" rot="R90">&gt;NAME</text>
<text x="-13.335" y="-1.016" size="1.778" layer="27">&gt;VALUE</text>
</package>
</packages>
<symbols>
<symbol name="DIL28">
<wire x1="-5.08" y1="16.51" x2="-5.08" y2="-19.05" width="0.254" layer="94"/>
<wire x1="-5.08" y1="-19.05" x2="5.08" y2="-19.05" width="0.254" layer="94"/>
<wire x1="5.08" y1="-19.05" x2="5.08" y2="16.51" width="0.254" layer="94"/>
<wire x1="5.08" y1="16.51" x2="2.54" y2="16.51" width="0.254" layer="94"/>
<wire x1="-5.08" y1="16.51" x2="-2.54" y2="16.51" width="0.254" layer="94"/>
<wire x1="-2.54" y1="16.51" x2="2.54" y2="16.51" width="0.254" layer="94" curve="180"/>
<text x="-4.445" y="17.145" size="1.778" layer="95">&gt;NAME</text>
<text x="-4.445" y="-21.59" size="1.778" layer="96">&gt;VALUE</text>
<pin name="1" x="-7.62" y="15.24" visible="pad" length="short" direction="pas"/>
<pin name="2" x="-7.62" y="12.7" visible="pad" length="short" direction="pas"/>
<pin name="3" x="-7.62" y="10.16" visible="pad" length="short" direction="pas"/>
<pin name="4" x="-7.62" y="7.62" visible="pad" length="short" direction="pas"/>
<pin name="5" x="-7.62" y="5.08" visible="pad" length="short" direction="pas"/>
<pin name="6" x="-7.62" y="2.54" visible="pad" length="short" direction="pas"/>
<pin name="7" x="-7.62" y="0" visible="pad" length="short" direction="pas"/>
<pin name="8" x="-7.62" y="-2.54" visible="pad" length="short" direction="pas"/>
<pin name="9" x="-7.62" y="-5.08" visible="pad" length="short" direction="pas"/>
<pin name="10" x="-7.62" y="-7.62" visible="pad" length="short" direction="pas"/>
<pin name="11" x="-7.62" y="-10.16" visible="pad" length="short" direction="pas"/>
<pin name="12" x="-7.62" y="-12.7" visible="pad" length="short" direction="pas"/>
<pin name="13" x="-7.62" y="-15.24" visible="pad" length="short" direction="pas"/>
<pin name="14" x="-7.62" y="-17.78" visible="pad" length="short" direction="pas"/>
<pin name="15" x="7.62" y="-17.78" visible="pad" length="short" direction="pas" rot="R180"/>
<pin name="16" x="7.62" y="-15.24" visible="pad" length="short" direction="pas" rot="R180"/>
<pin name="17" x="7.62" y="-12.7" visible="pad" length="short" direction="pas" rot="R180"/>
<pin name="18" x="7.62" y="-10.16" visible="pad" length="short" direction="pas" rot="R180"/>
<pin name="19" x="7.62" y="-7.62" visible="pad" length="short" direction="pas" rot="R180"/>
<pin name="20" x="7.62" y="-5.08" visible="pad" length="short" direction="pas" rot="R180"/>
<pin name="21" x="7.62" y="-2.54" visible="pad" length="short" direction="pas" rot="R180"/>
<pin name="22" x="7.62" y="0" visible="pad" length="short" direction="pas" rot="R180"/>
<pin name="23" x="7.62" y="2.54" visible="pad" length="short" direction="pas" rot="R180"/>
<pin name="24" x="7.62" y="5.08" visible="pad" length="short" direction="pas" rot="R180"/>
<pin name="25" x="7.62" y="7.62" visible="pad" length="short" direction="pas" rot="R180"/>
<pin name="26" x="7.62" y="10.16" visible="pad" length="short" direction="pas" rot="R180"/>
<pin name="27" x="7.62" y="12.7" visible="pad" length="short" direction="pas" rot="R180"/>
<pin name="28" x="7.62" y="15.24" visible="pad" length="short" direction="pas" rot="R180"/>
</symbol>
</symbols>
<devicesets>
<deviceset name="DIL28" prefix="IC" uservalue="yes">
<description>&lt;b&gt;Dual In Line&lt;/b&gt;</description>
<gates>
<gate name="G$1" symbol="DIL28" x="0" y="0"/>
</gates>
<devices>
<device name="" package="DIL28-6">
<connects>
<connect gate="G$1" pin="1" pad="1"/>
<connect gate="G$1" pin="10" pad="10"/>
<connect gate="G$1" pin="11" pad="11"/>
<connect gate="G$1" pin="12" pad="12"/>
<connect gate="G$1" pin="13" pad="13"/>
<connect gate="G$1" pin="14" pad="14"/>
<connect gate="G$1" pin="15" pad="15"/>
<connect gate="G$1" pin="16" pad="16"/>
<connect gate="G$1" pin="17" pad="17"/>
<connect gate="G$1" pin="18" pad="18"/>
<connect gate="G$1" pin="19" pad="19"/>
<connect gate="G$1" pin="2" pad="2"/>
<connect gate="G$1" pin="20" pad="20"/>
<connect gate="G$1" pin="21" pad="21"/>
<connect gate="G$1" pin="22" pad="22"/>
<connect gate="G$1" pin="23" pad="23"/>
<connect gate="G$1" pin="24" pad="24"/>
<connect gate="G$1" pin="25" pad="25"/>
<connect gate="G$1" pin="26" pad="26"/>
<connect gate="G$1" pin="27" pad="27"/>
<connect gate="G$1" pin="28" pad="28"/>
<connect gate="G$1" pin="3" pad="3"/>
<connect gate="G$1" pin="4" pad="4"/>
<connect gate="G$1" pin="5" pad="5"/>
<connect gate="G$1" pin="6" pad="6"/>
<connect gate="G$1" pin="7" pad="7"/>
<connect gate="G$1" pin="8" pad="8"/>
<connect gate="G$1" pin="9" pad="9"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
<device name="-3" package="DIL28-3">
<connects>
<connect gate="G$1" pin="1" pad="1"/>
<connect gate="G$1" pin="10" pad="10"/>
<connect gate="G$1" pin="11" pad="11"/>
<connect gate="G$1" pin="12" pad="12"/>
<connect gate="G$1" pin="13" pad="13"/>
<connect gate="G$1" pin="14" pad="14"/>
<connect gate="G$1" pin="15" pad="15"/>
<connect gate="G$1" pin="16" pad="16"/>
<connect gate="G$1" pin="17" pad="17"/>
<connect gate="G$1" pin="18" pad="18"/>
<connect gate="G$1" pin="19" pad="19"/>
<connect gate="G$1" pin="2" pad="2"/>
<connect gate="G$1" pin="20" pad="20"/>
<connect gate="G$1" pin="21" pad="21"/>
<connect gate="G$1" pin="22" pad="22"/>
<connect gate="G$1" pin="23" pad="23"/>
<connect gate="G$1" pin="24" pad="24"/>
<connect gate="G$1" pin="25" pad="25"/>
<connect gate="G$1" pin="26" pad="26"/>
<connect gate="G$1" pin="27" pad="27"/>
<connect gate="G$1" pin="28" pad="28"/>
<connect gate="G$1" pin="3" pad="3"/>
<connect gate="G$1" pin="4" pad="4"/>
<connect gate="G$1" pin="5" pad="5"/>
<connect gate="G$1" pin="6" pad="6"/>
<connect gate="G$1" pin="7" pad="7"/>
<connect gate="G$1" pin="8" pad="8"/>
<connect gate="G$1" pin="9" pad="9"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
<device name="-4" package="DIL28-4">
<connects>
<connect gate="G$1" pin="1" pad="1"/>
<connect gate="G$1" pin="10" pad="10"/>
<connect gate="G$1" pin="11" pad="11"/>
<connect gate="G$1" pin="12" pad="12"/>
<connect gate="G$1" pin="13" pad="13"/>
<connect gate="G$1" pin="14" pad="14"/>
<connect gate="G$1" pin="15" pad="15"/>
<connect gate="G$1" pin="16" pad="16"/>
<connect gate="G$1" pin="17" pad="17"/>
<connect gate="G$1" pin="18" pad="18"/>
<connect gate="G$1" pin="19" pad="19"/>
<connect gate="G$1" pin="2" pad="2"/>
<connect gate="G$1" pin="20" pad="20"/>
<connect gate="G$1" pin="21" pad="21"/>
<connect gate="G$1" pin="22" pad="22"/>
<connect gate="G$1" pin="23" pad="23"/>
<connect gate="G$1" pin="24" pad="24"/>
<connect gate="G$1" pin="25" pad="25"/>
<connect gate="G$1" pin="26" pad="26"/>
<connect gate="G$1" pin="27" pad="27"/>
<connect gate="G$1" pin="28" pad="28"/>
<connect gate="G$1" pin="3" pad="3"/>
<connect gate="G$1" pin="4" pad="4"/>
<connect gate="G$1" pin="5" pad="5"/>
<connect gate="G$1" pin="6" pad="6"/>
<connect gate="G$1" pin="7" pad="7"/>
<connect gate="G$1" pin="8" pad="8"/>
<connect gate="G$1" pin="9" pad="9"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
</devicesets>
</library>
<library name="supply1">
<description>&lt;b&gt;Supply Symbols&lt;/b&gt;&lt;p&gt;
 GND, VCC, 0V, +5V, -5V, etc.&lt;p&gt;
 Please keep in mind, that these devices are necessary for the
 automatic wiring of the supply signals.&lt;p&gt;
 The pin name defined in the symbol is identical to the net which is to be wired automatically.&lt;p&gt;
 In this library the device names are the same as the pin names of the symbols, therefore the correct signal names appear next to the supply symbols in the schematic.&lt;p&gt;
 &lt;author&gt;Created by librarian@cadsoft.de&lt;/author&gt;</description>
<packages>
</packages>
<symbols>
<symbol name="GND">
<wire x1="-1.905" y1="0" x2="1.905" y2="0" width="0.254" layer="94"/>
<text x="-2.54" y="-2.54" size="1.778" layer="96">&gt;VALUE</text>
<pin name="GND" x="0" y="2.54" visible="off" length="short" direction="sup" rot="R270"/>
</symbol>
</symbols>
<devicesets>
<deviceset name="GND" prefix="GND">
<description>&lt;b&gt;SUPPLY SYMBOL&lt;/b&gt;</description>
<gates>
<gate name="1" symbol="GND" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
</devicesets>
</library>
</libraries>
<attributes>
</attributes>
<variantdefs>
</variantdefs>
<classes>
<class number="0" name="default" width="0" drill="0">
</class>
</classes>
<parts>
<part name="BREAKOUT" library="texas" deviceset="DRV8824" device="PWP"/>
<part name="IC2" library="ic-package" deviceset="DIL28" device=""/>
<part name="GND1" library="supply1" deviceset="GND" device=""/>
</parts>
<sheets>
<sheet>
<plain>
</plain>
<instances>
<instance part="BREAKOUT" gate="M1" x="25.4" y="68.58" rot="R270"/>
<instance part="IC2" gate="G$1" x="71.12" y="68.58" rot="MR270"/>
<instance part="GND1" gate="1" x="50.8" y="119.38" rot="R180"/>
</instances>
<busses>
</busses>
<nets>
<net name="V3P3" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="V3P3"/>
<pinref part="IC2" gate="G$1" pin="15"/>
<wire x1="88.9" y1="60.96" x2="88.9" y2="22.86" width="0.1524" layer="91"/>
<wire x1="88.9" y1="22.86" x2="10.16" y2="22.86" width="0.1524" layer="91"/>
<wire x1="10.16" y1="22.86" x2="10.16" y2="55.88" width="0.1524" layer="91"/>
</segment>
</net>
<net name="!RES" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="!RES"/>
<pinref part="IC2" gate="G$1" pin="16"/>
<wire x1="86.36" y1="60.96" x2="86.36" y2="25.4" width="0.1524" layer="91"/>
<wire x1="86.36" y1="25.4" x2="12.7" y2="25.4" width="0.1524" layer="91"/>
<wire x1="12.7" y1="25.4" x2="12.7" y2="55.88" width="0.1524" layer="91"/>
</segment>
</net>
<net name="!SLP" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="!SLP"/>
<pinref part="IC2" gate="G$1" pin="17"/>
<wire x1="83.82" y1="60.96" x2="83.82" y2="27.94" width="0.1524" layer="91"/>
<wire x1="83.82" y1="27.94" x2="15.24" y2="27.94" width="0.1524" layer="91"/>
<wire x1="15.24" y1="27.94" x2="15.24" y2="55.88" width="0.1524" layer="91"/>
</segment>
</net>
<net name="!ERR" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="!ERR"/>
<pinref part="IC2" gate="G$1" pin="18"/>
<wire x1="81.28" y1="60.96" x2="81.28" y2="30.48" width="0.1524" layer="91"/>
<wire x1="81.28" y1="30.48" x2="17.78" y2="30.48" width="0.1524" layer="91"/>
<wire x1="17.78" y1="30.48" x2="17.78" y2="55.88" width="0.1524" layer="91"/>
</segment>
</net>
<net name="DECAY" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="DECAY"/>
<pinref part="IC2" gate="G$1" pin="19"/>
<wire x1="78.74" y1="60.96" x2="78.74" y2="33.02" width="0.1524" layer="91"/>
<wire x1="78.74" y1="33.02" x2="20.32" y2="33.02" width="0.1524" layer="91"/>
<wire x1="20.32" y1="33.02" x2="20.32" y2="55.88" width="0.1524" layer="91"/>
</segment>
</net>
<net name="DIR" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="DIR"/>
<pinref part="IC2" gate="G$1" pin="20"/>
<wire x1="76.2" y1="60.96" x2="76.2" y2="35.56" width="0.1524" layer="91"/>
<wire x1="76.2" y1="35.56" x2="22.86" y2="35.56" width="0.1524" layer="91"/>
<wire x1="22.86" y1="35.56" x2="22.86" y2="55.88" width="0.1524" layer="91"/>
</segment>
</net>
<net name="!EN" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="!EN"/>
<pinref part="IC2" gate="G$1" pin="21"/>
<wire x1="73.66" y1="60.96" x2="73.66" y2="38.1" width="0.1524" layer="91"/>
<wire x1="73.66" y1="38.1" x2="25.4" y2="38.1" width="0.1524" layer="91"/>
<wire x1="25.4" y1="38.1" x2="25.4" y2="55.88" width="0.1524" layer="91"/>
</segment>
</net>
<net name="STEP" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="STEP"/>
<pinref part="IC2" gate="G$1" pin="22"/>
<wire x1="71.12" y1="60.96" x2="71.12" y2="40.64" width="0.1524" layer="91"/>
<wire x1="71.12" y1="40.64" x2="27.94" y2="40.64" width="0.1524" layer="91"/>
<wire x1="27.94" y1="40.64" x2="27.94" y2="55.88" width="0.1524" layer="91"/>
</segment>
</net>
<net name="MODE0" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="MODE0"/>
<pinref part="IC2" gate="G$1" pin="24"/>
<wire x1="66.04" y1="60.96" x2="66.04" y2="45.72" width="0.1524" layer="91"/>
<wire x1="66.04" y1="45.72" x2="33.02" y2="45.72" width="0.1524" layer="91"/>
<wire x1="33.02" y1="45.72" x2="33.02" y2="55.88" width="0.1524" layer="91"/>
</segment>
</net>
<net name="MODE1" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="MODE1"/>
<pinref part="IC2" gate="G$1" pin="25"/>
<wire x1="63.5" y1="60.96" x2="63.5" y2="48.26" width="0.1524" layer="91"/>
<wire x1="63.5" y1="48.26" x2="35.56" y2="48.26" width="0.1524" layer="91"/>
<wire x1="35.56" y1="48.26" x2="35.56" y2="55.88" width="0.1524" layer="91"/>
</segment>
</net>
<net name="MODE2" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="MODE2"/>
<pinref part="IC2" gate="G$1" pin="26"/>
<wire x1="60.96" y1="60.96" x2="60.96" y2="50.8" width="0.1524" layer="91"/>
<wire x1="60.96" y1="50.8" x2="38.1" y2="50.8" width="0.1524" layer="91"/>
<wire x1="38.1" y1="50.8" x2="38.1" y2="55.88" width="0.1524" layer="91"/>
</segment>
</net>
<net name="!HOME" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="!HOME"/>
<pinref part="IC2" gate="G$1" pin="27"/>
<wire x1="58.42" y1="60.96" x2="58.42" y2="53.34" width="0.1524" layer="91"/>
<wire x1="58.42" y1="53.34" x2="40.64" y2="53.34" width="0.1524" layer="91"/>
<wire x1="40.64" y1="53.34" x2="40.64" y2="55.88" width="0.1524" layer="91"/>
</segment>
</net>
<net name="CP1" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="CP1"/>
<pinref part="IC2" gate="G$1" pin="1"/>
<wire x1="55.88" y1="76.2" x2="55.88" y2="81.28" width="0.1524" layer="91"/>
<wire x1="55.88" y1="81.28" x2="43.18" y2="81.28" width="0.1524" layer="91"/>
</segment>
</net>
<net name="CP2" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="CP2"/>
<pinref part="IC2" gate="G$1" pin="2"/>
<wire x1="58.42" y1="76.2" x2="58.42" y2="83.82" width="0.1524" layer="91"/>
<wire x1="58.42" y1="83.82" x2="40.64" y2="83.82" width="0.1524" layer="91"/>
<wire x1="40.64" y1="83.82" x2="40.64" y2="81.28" width="0.1524" layer="91"/>
</segment>
</net>
<net name="VCP" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="VCP"/>
<pinref part="IC2" gate="G$1" pin="3"/>
<wire x1="60.96" y1="76.2" x2="60.96" y2="86.36" width="0.1524" layer="91"/>
<wire x1="60.96" y1="86.36" x2="38.1" y2="86.36" width="0.1524" layer="91"/>
<wire x1="38.1" y1="86.36" x2="38.1" y2="81.28" width="0.1524" layer="91"/>
</segment>
</net>
<net name="VMA" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="VMA"/>
<pinref part="IC2" gate="G$1" pin="4"/>
<wire x1="63.5" y1="76.2" x2="63.5" y2="88.9" width="0.1524" layer="91"/>
<wire x1="63.5" y1="88.9" x2="35.56" y2="88.9" width="0.1524" layer="91"/>
<wire x1="35.56" y1="88.9" x2="35.56" y2="81.28" width="0.1524" layer="91"/>
</segment>
</net>
<net name="AOUT1" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="AOUT1"/>
<pinref part="IC2" gate="G$1" pin="5"/>
<wire x1="66.04" y1="76.2" x2="66.04" y2="91.44" width="0.1524" layer="91"/>
<wire x1="66.04" y1="91.44" x2="33.02" y2="91.44" width="0.1524" layer="91"/>
<wire x1="33.02" y1="91.44" x2="33.02" y2="81.28" width="0.1524" layer="91"/>
</segment>
</net>
<net name="ISENA" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="ISENA"/>
<pinref part="IC2" gate="G$1" pin="6"/>
<wire x1="68.58" y1="76.2" x2="68.58" y2="93.98" width="0.1524" layer="91"/>
<wire x1="68.58" y1="93.98" x2="30.48" y2="93.98" width="0.1524" layer="91"/>
<wire x1="30.48" y1="93.98" x2="30.48" y2="81.28" width="0.1524" layer="91"/>
</segment>
</net>
<net name="AOUT2" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="AOUT2"/>
<pinref part="IC2" gate="G$1" pin="7"/>
<wire x1="71.12" y1="76.2" x2="71.12" y2="96.52" width="0.1524" layer="91"/>
<wire x1="71.12" y1="96.52" x2="27.94" y2="96.52" width="0.1524" layer="91"/>
<wire x1="27.94" y1="96.52" x2="27.94" y2="81.28" width="0.1524" layer="91"/>
</segment>
</net>
<net name="BOUT2" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="BOUT2"/>
<pinref part="IC2" gate="G$1" pin="8"/>
<wire x1="73.66" y1="76.2" x2="73.66" y2="99.06" width="0.1524" layer="91"/>
<wire x1="73.66" y1="99.06" x2="25.4" y2="99.06" width="0.1524" layer="91"/>
<wire x1="25.4" y1="99.06" x2="25.4" y2="81.28" width="0.1524" layer="91"/>
</segment>
</net>
<net name="ISENB" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="ISENB"/>
<pinref part="IC2" gate="G$1" pin="9"/>
<wire x1="76.2" y1="76.2" x2="76.2" y2="101.6" width="0.1524" layer="91"/>
<wire x1="76.2" y1="101.6" x2="22.86" y2="101.6" width="0.1524" layer="91"/>
<wire x1="22.86" y1="101.6" x2="22.86" y2="81.28" width="0.1524" layer="91"/>
</segment>
</net>
<net name="BOUT1" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="BOUT1"/>
<pinref part="IC2" gate="G$1" pin="10"/>
<wire x1="78.74" y1="76.2" x2="78.74" y2="104.14" width="0.1524" layer="91"/>
<wire x1="78.74" y1="104.14" x2="20.32" y2="104.14" width="0.1524" layer="91"/>
<wire x1="20.32" y1="104.14" x2="20.32" y2="81.28" width="0.1524" layer="91"/>
</segment>
</net>
<net name="VMB" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="VMB"/>
<pinref part="IC2" gate="G$1" pin="11"/>
<wire x1="81.28" y1="76.2" x2="81.28" y2="106.68" width="0.1524" layer="91"/>
<wire x1="81.28" y1="106.68" x2="17.78" y2="106.68" width="0.1524" layer="91"/>
<wire x1="17.78" y1="106.68" x2="17.78" y2="81.28" width="0.1524" layer="91"/>
</segment>
</net>
<net name="AVREF" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="AVREF"/>
<pinref part="IC2" gate="G$1" pin="12"/>
<wire x1="83.82" y1="76.2" x2="83.82" y2="109.22" width="0.1524" layer="91"/>
<wire x1="83.82" y1="109.22" x2="15.24" y2="109.22" width="0.1524" layer="91"/>
<wire x1="15.24" y1="109.22" x2="15.24" y2="81.28" width="0.1524" layer="91"/>
</segment>
</net>
<net name="BVREF" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="BVREF"/>
<pinref part="IC2" gate="G$1" pin="13"/>
<wire x1="86.36" y1="76.2" x2="86.36" y2="111.76" width="0.1524" layer="91"/>
<wire x1="86.36" y1="111.76" x2="12.7" y2="111.76" width="0.1524" layer="91"/>
<wire x1="12.7" y1="111.76" x2="12.7" y2="81.28" width="0.1524" layer="91"/>
</segment>
</net>
<net name="GND" class="0">
<segment>
<pinref part="BREAKOUT" gate="M1" pin="GND2"/>
<pinref part="IC2" gate="G$1" pin="28"/>
<wire x1="55.88" y1="60.96" x2="55.88" y2="55.88" width="0.1524" layer="91"/>
<wire x1="55.88" y1="55.88" x2="50.8" y2="55.88" width="0.1524" layer="91"/>
<pinref part="BREAKOUT" gate="M1" pin="PAD"/>
<wire x1="50.8" y1="55.88" x2="43.18" y2="55.88" width="0.1524" layer="91"/>
<wire x1="48.26" y1="68.58" x2="50.8" y2="68.58" width="0.1524" layer="91"/>
<wire x1="50.8" y1="68.58" x2="50.8" y2="55.88" width="0.1524" layer="91"/>
<junction x="50.8" y="55.88"/>
<junction x="50.8" y="68.58"/>
<pinref part="BREAKOUT" gate="M1" pin="GND1"/>
<pinref part="IC2" gate="G$1" pin="14"/>
<wire x1="88.9" y1="76.2" x2="88.9" y2="114.3" width="0.1524" layer="91"/>
<wire x1="88.9" y1="114.3" x2="50.8" y2="114.3" width="0.1524" layer="91"/>
<wire x1="50.8" y1="114.3" x2="10.16" y2="114.3" width="0.1524" layer="91"/>
<wire x1="10.16" y1="114.3" x2="10.16" y2="81.28" width="0.1524" layer="91"/>
<wire x1="50.8" y1="68.58" x2="50.8" y2="114.3" width="0.1524" layer="91"/>
<junction x="50.8" y="114.3"/>
<pinref part="GND1" gate="1" pin="GND"/>
<wire x1="50.8" y1="116.84" x2="50.8" y2="114.3" width="0.1524" layer="91"/>
</segment>
</net>
</nets>
</sheet>
</sheets>
</schematic>
</drawing>
</eagle>
