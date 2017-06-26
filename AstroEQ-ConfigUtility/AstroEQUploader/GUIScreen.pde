import java.util.ListIterator;

class GUIScreen {
  private ControlGroup _group;
  
  private String _groupID;
  
  private Dimensions _dimension;
  
  protected LinkedHashMap<String,Button> buttonList = new LinkedHashMap<String,Button>();
  protected LinkedHashMap<String,Slider> sliderList = new LinkedHashMap<String,Slider>();
  protected LinkedHashMap<String,Textlabel> labelList = new LinkedHashMap<String,Textlabel>();
  protected LinkedHashMap<String,Toggle> toggleList = new LinkedHashMap<String,Toggle>();
  protected LinkedHashMap<String,ScrollableList> dropdownList = new LinkedHashMap<String,ScrollableList>();
  protected LinkedHashMap<String,Textfield> fieldList = new LinkedHashMap<String,Textfield>();
  
  public int button_color         = color(0,97,255);
  public int button_active_color  = color(255,192,0);
  public int slider_color         = color(255,192,0);
  
  GUIScreen (ControlP5 control, String groupID, String pageTitle, Dimensions dimension) {
    _groupID = groupID;
    _dimension = dimension;
    _group = control.addGroup(groupID,dimension.x(),dimension.y(),dimension.width());
    int count = 7 * pageTitle.length()/2;
    Dimensions labelSize = new Dimensions(0,0,(dimension.width()/2) - count,0,count*2,20);
    addTextLabel(control,"title",wl_font,pageTitle,color(#ffffff),labelSize);
    _group.hideBar();
    _group.show();
    _group.hide();
  }
  
  public void hideGroup() {
    Iterator it = fieldList.entrySet().iterator();
    while (it.hasNext()) {
      Map.Entry pairs = (Map.Entry)it.next();
      Textfield field = (Textfield)pairs.getValue();
      field.setVisible(false);
    }
    it = dropdownList.entrySet().iterator();
    while (it.hasNext()) {
      Map.Entry pairs = (Map.Entry)it.next();
      ScrollableList dropdown = (ScrollableList)pairs.getValue();
      dropdown.setVisible(false);
    }
    _group.hide();
  }
  
  public void showGroup() {
    Iterator it = fieldList.entrySet().iterator();
    while (it.hasNext()) {
      Map.Entry pairs = (Map.Entry)it.next();
      Textfield field = (Textfield)pairs.getValue();
      field.setVisible(true);
    }
    it = dropdownList.entrySet().iterator();
    while (it.hasNext()) {
      Map.Entry pairs = (Map.Entry)it.next();
      ScrollableList dropdown = (ScrollableList)pairs.getValue();
      dropdown.setVisible(true);
    }
    _group.show();
  }
  
  public String getTextOfFocusTextField() {
    Iterator it = fieldList.entrySet().iterator();
    while (it.hasNext()) {
      Map.Entry pairs = (Map.Entry)it.next();
      Textfield field = (Textfield)pairs.getValue();
      if (field.isFocus()) {
        return field.getText();
      }
    }
    return "";
  }
  
  public void setTextOfFocusTextField(String text) {
    Iterator it = fieldList.entrySet().iterator();
    while (it.hasNext()) {
      Map.Entry pairs = (Map.Entry)it.next();
      Textfield field = (Textfield)pairs.getValue();
      if (field.isFocus()) {
        field.setText(text);
        return;
      }
    }
  }
  
  public void focusNextTextField() {
    ListIterator it = new ArrayList<Map.Entry>(fieldList.entrySet()).listIterator();
    if (!it.hasNext()) {
      return; //no fields.
    }
    Textfield firstField = null;
    Textfield nextField = null;
    Textfield field = null;
    
    while (it.hasNext()) {
      Map.Entry pairs = (Map.Entry)it.next();
      nextField = (Textfield)pairs.getValue(); //get the next field
      if (nextField.isLock()) {
        continue; //skip locked fields.
      }
      if ((field != null) && field.isFocus()) { //if this field is focus
        field.setFocus(false); //clear the focus
        nextField.setFocus(true); //and focus on the next field
        return; //finished.
      }
      field = nextField; //Next is now the current one to test.
      if (firstField == null) {
        firstField = field; //Keep track of the first field that the iterator has
      }
    }
    if ((field != null) && field.isFocus()) { //if the final field in the iterator is focus
      field.setFocus(false); //clear the focus
      if (firstField != null) {
        firstField.setFocus(true); //and focus on the first field
      }
    }
  }
  
  public void focusPreviousTextField() {
    ListIterator it = new ArrayList<Map.Entry>(fieldList.entrySet()).listIterator(fieldList.entrySet().size());
    if (!it.hasPrevious()) {
      return; //no fields.
    }
    Textfield lastField = null;
    Textfield prevField = null;
    Textfield field = null;
    
    while (it.hasPrevious()) {
      Map.Entry pairs = (Map.Entry)it.previous();
      prevField = (Textfield)pairs.getValue(); //get the next field
      if (prevField.isLock()) {
        continue; //skip locked fields.
      }
      if ((field != null) && field.isFocus()) { //if this field is focus
        field.setFocus(false); //clear the focus
        prevField.setFocus(true); //and focus on the next field
        return; //finished.
      }
      field = prevField; //Next is now the current one to test.
      if (lastField == null) {
        lastField = field; //Keep track of the first field that the iterator has
      }
    }
    if ((field != null) && field.isFocus()) { //if the final field in the iterator is focus
      field.setFocus(false); //clear the focus
      if (lastField != null) {
        lastField.setFocus(true); //and focus on the first field
      }
    }
  }
  
  public ControlGroup getGroup(){
    return _group;
  }
  public String getGroupID(){
    return _groupID;
  }
  
  public Slider addSlider(ControlP5 panel, String s_name, float minv, float maxv, float def, Dimensions dimension) {
    
    Slider s = panel.addSlider(_groupID+s_name, minv, maxv, def, dimension.x(), dimension.y(), dimension.width(), dimension.height());
    s.setGroup(_group);
    s.setLabel("");
    s.setColorBackground(button_color);
    s.setColorActive(button_active_color);
    s.setColorForeground(slider_color);
    
    sliderList.put(s_name,s);
    return s;
  }
  public Slider getSlider( String s_name ){
    return getSlider("",s_name);
  }
  public Slider getSlider( String id, String s_name ){
    if( sliderList.containsKey(id+s_name) ) {
      return sliderList.get(id+s_name);
    } else {
      return null;
    }
  }
  
  public Textlabel addTextLabel( ControlP5 panel, String l_name, PFont font, String val, int colour, Dimensions dimension) {
    Textlabel l = panel.addTextlabel(_groupID+l_name)
                       .setText(val)
                       .setPosition(dimension.x(),dimension.y())
                       .setColorValue(colour)
                       .setFont(font)
                       .setWidth(dimension.width())
                       ;
    l.setGroup(_group);
    
    labelList.put(l_name,l);
    return l;
  }
  public Textlabel getTextLabel( String l_name ){
    return getTextLabel("",l_name);
  }
  public Textlabel getTextLabel( String id, String l_name ){
    if( labelList.containsKey(id+l_name) ) {
      return labelList.get(id+l_name);
    } else {
      return null;
    }
  }

  public Toggle addToggle( ControlP5 panel, String t_name, Boolean val, Dimensions dimension){
    
    Toggle t = new Toggle(panel, _groupID+t_name);
    t.setValue(val);
    t.setPosition(dimension.x(),dimension.y());
    t.setSize(dimension.width(), dimension.height());
        
    t.setLabelVisible(false);
    t.setGroup(_group);
    t.setColorBackground(button_color);
    t.setColorForeground(button_color);
    t.setColorActive(button_active_color);
    
    toggleList.put(t_name,t);
    return t;
  }
  public Toggle getToggle( String t_name ){
    return getToggle("",t_name);
  }
  public Toggle getToggle( String id, String t_name ){
    if( toggleList.containsKey(id+t_name) ) {
      return toggleList.get(id+t_name);
    } else {
      return null;
    }
  }

  public ScrollableList addDropdown( ControlP5 panel, String d_name, String caption, PFont font, Map<String,Integer> entries, Dimensions dimension){
    
    dimension = dimension.clone();
    dimension.shift(0,-dimension.height()); //shift up by requested height to account for weirdness if the origin of the dropdowns.
    
    ScrollableList d = panel.addScrollableList(_groupID+d_name, dimension.x()+_dimension.x(), dimension.y()+_dimension.y()+dimension.height(), dimension.width(),7*dimension.height());
    d.setBackgroundColor(color(0));
    d.setItemHeight(dimension.height());
    d.setBarHeight(dimension.height());
    d.getCaptionLabel().getStyle().marginTop = 4;
    d.getCaptionLabel().getStyle().marginLeft = 2;
    d.getCaptionLabel().setFont(font);
    d.getCaptionLabel().set(caption);
    d.getCaptionLabel().toUpperCase(false);
    
    for (Map.Entry entry : entries.entrySet()) {
      d.addItem((String)entry.getKey(), ((Integer)entry.getValue()).intValue());
    }
        
    d.setValue(0);
    //d.setGroup(_group);
    d.setColorBackground(button_color);
    d.setColorActive(button_active_color);
    d.setColorForeground(button_active_color);
    
    d.close();
    
    dropdownList.put(d_name,d);
    return d;
  }
  public ScrollableList getDropdown( String d_name ){
    return getDropdown("",d_name);
  }
  public ScrollableList getDropdown( String id, String d_name ){
    if( dropdownList.containsKey(id+d_name) ) {
      return dropdownList.get(id+d_name);
    } else {
      return null;
    }
  }
  
  public Object getScrollableListItem( String name ) {
    return getScrollableListItem(name, "value");
  }
  
  public Object getScrollableListItem( String name, String field) {
    ScrollableList ddl = getDropdown(name);
    return getScrollableListItem(ddl,field);
  }
  
  public Object getScrollableListItem( ScrollableList ddl ) {
    return getScrollableListItem( ddl, "value");
  }
    
  public Object getScrollableListItem( ScrollableList ddl, String field) {
    if (ddl == null) {
      return null;
    }
    Map<String,Object> item = ddl.getItem((int)ddl.getValue());
    return item.get(field);
  }
  
  public Object getScrollableListItem( ScrollableList ddl, String item, String field) {
    if (ddl == null) {
      return null;
    }
    Map<String,Object> obj = ddl.getItem(item);
    return obj.get(field);
  }
    
  public void selectScrollableListItem( String name, Object value) {
    selectScrollableListItem( name, "value", value);
  }
    
  public void selectScrollableListItem( String name, String field, Object value) {
    ScrollableList ddl = getDropdown(name);
    selectScrollableListItem( ddl, field, value);
  }
    
  public void selectScrollableListItem( ScrollableList ddl, Object value) {
    selectScrollableListItem( ddl, "value", value);
  }
  
  public void selectScrollableListItem( ScrollableList ddl, String field, Object value) {
    if (ddl == null) {
      return;
    }
    List items = ddl.getItems();
    for (int i = 0; i < items.size(); i++) {
      Map<String,Object> item = (Map<String,Object>)items.get(i);
      Object obj = item.get(field);
      if (obj.equals(value)) {
        ddl.setValue(i);
        return;
      }
    }
  }

  
  public Button addButton( ControlP5 panel, String b_name, String b_label, PFont font, int col, Dimensions dimension) {
    
    Button b = new Button(panel, _groupID+b_name);
    b.setValue(col);
    b.setPosition(dimension.x(),dimension.y());
    b.setSize(dimension.width(), dimension.height());
    b.setColorBackground(button_color);
    b.setColorActive(button_active_color);
    b.setGroup(_group);
    b.setCaptionLabel(b_label);
    
    b.getCaptionLabel().setFont(new ControlFont(font));
    b.getCaptionLabel().toUpperCase(false);
    b.getCaptionLabel().getStyle().marginTop = -1;
    
    buttonList.put(b_name,b);
    return b;
  }
  public Button getButton( String b_name ){
    return getButton("",b_name);
  }
  public Button getButton( String id, String b_name ){
    if( buttonList.containsKey(id+b_name) ) {
      return buttonList.get(id+b_name);
    } else {
      return null;
    }
  }
  
  public Textfield addField( ControlP5 panel, String f_name, int filt, PFont font, int colour, boolean backgnd, Dimensions dimension) {
    
    dimension = dimension.clone();
    dimension.expand(-1,0); //Shrink width of the dimensions to account for incorrect 1px border.
    
    Textfield f = panel.addTextfield(_groupID+f_name, _dimension.x() + dimension.x(), _dimension.y() + dimension.y(), dimension.width(), dimension.height());
    
    if(!backgnd) {
      f.setColorBackground(0x00FFFFFF);
    } else {
      f.setColorBackground(0xFFFFFFFF);
    }
    f.setColorForeground(button_color);
    f.setColor(colour);
    f.setColorCursor(colour);
    //f.cursorWidth = 2;
    f.setFont(font);
    f.setInputFilter(filt);
    f.setAutoClear(false);
    
    f.setVisible(false);
    
    f.getCaptionLabel().setVisible(false);
    
    fieldList.put(f_name,f);
    return f;
  }
  public Textfield getField( String f_name ){
    return getField("",f_name);
  }
  public Textfield getField( String id, String f_name ){
    if( fieldList.containsKey(id+f_name) ) {
      return fieldList.get(id+f_name);
    } else {
      return null;
    }
  }
  
}