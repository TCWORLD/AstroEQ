static class SyntaString {
        
    public final static boolean encode = true;
    public final static boolean decode = false;
    
    public final static int argIsBool = 1;
    public final static int argIsByte = 2;
    public final static int argIsLong = 6;
    
    public static String syntaEncoder(String str, int length, boolean mode){
      if (mode == encode){
        if (length == argIsBool) {
          int data = Integer.valueOf(str);
          data = (data != 0) ? 1 : 0;
          str = String.format("%01X", data);
        } else if (length == argIsByte){
          int data = Integer.valueOf(str);
          data = data & 0xFF;
          str = String.format("%02X", data);
        } else if (length == argIsLong){
          long data = Long.valueOf(str);
          int[] bytes = new int[3];
          bytes[0] = (int)(data >>> 16) & 0xFF;
          bytes[1] = (int)(data >>> 8) & 0xFF;
          bytes[2] = (int)(data) & 0xFF;
          str = String.format("%02X%02X%02X", bytes[2],bytes[1],bytes[0]);
        }
      } else {
        if (length == argIsBool) {
          if (str.length() != 1) {
            return "0";
          }
          int data = Integer.parseInt(str, 16);
          data = (data != 0) ? 1 : 0;
          str = Integer.toString(data);
        } else if (length == argIsByte){
          if (str.length() != 2) {
            return "0";
          }
          int data = Integer.parseInt(str, 16);
          data = data & 0xFF;
          str = Integer.toString(data);
        } else if (length == argIsLong){
          if (str.length() != 6) {
            return "0";
          }
          
          int[] bytes = new int[3];
          for (byte i = 0; i < 3; i++){
            bytes[i] = Integer.parseInt(str.substring(2*i,2*(i+1)), 16);
            bytes[i] = bytes[i] & 0xFF;
          }
          
          long data = (bytes[0]) + (bytes[1] << 8) + (bytes[2] << 16);
          str = Long.toString(data);
        }
      }      
      return str;
    }
  }