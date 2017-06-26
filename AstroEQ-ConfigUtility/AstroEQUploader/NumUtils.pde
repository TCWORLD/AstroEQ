//From here: http://stackoverflow.com/questions/1102891/how-to-check-if-a-string-is-numeric-in-java
static public class NumUtils {
    /**
     * Transforms a string to an integer. If no numerical chars returns a String "0".
     *
     * @param str
     * @return retStr
     */
    static String makeToInteger(String str) {
        String s = str;
        double d;
        d = Double.parseDouble(makeToDouble(s));
        int i = (int) (d + 0.5D);
        String retStr = String.valueOf(i);
        System.out.printf(retStr + "   ");
        return retStr;
    }

    /**
     * Transforms a string to an double. If no numerical chars returns a String "0".
     *
     * @param str
     * @return retStr
     */
    static String makeToDouble(String str) {

        Boolean dotWasFound = false;
        //String orgStr = str;
        String retStr;
        int firstDotPos = 0;
        Boolean negative = false;

        //check if str is null
        if(str.length()==0){
            str="0";
        }

        //check if first sign is "-"
        if (str.charAt(0) == '-') {
            negative = true;
        }

        //check if str containg any number or else set the string to '0'
        if (!str.matches(".*\\d+.*")) {
            str = "0";
        }

        //Replace ',' with '.'  (for some european users who use the ',' as decimal separator)
        str = str.replaceAll(",", ".");
        str = str.replaceAll("[^\\d.]", "");

        //Removes the any second dots
        for (int i_char = 0; i_char < str.length(); i_char++) {
            if (str.charAt(i_char) == '.') {
                dotWasFound = true;
                firstDotPos = i_char;
                break;
            }
        }
        if (dotWasFound) {
            String befDot = str.substring(0, firstDotPos + 1);
            String aftDot = str.substring(firstDotPos + 1, str.length());
            aftDot = aftDot.replaceAll("\\.", "");
            str = befDot + aftDot;
        }

        //Removes zeros from the begining
        double uglyMethod = Double.parseDouble(str);
        str = String.valueOf(uglyMethod);

        //Removes the .0
        str = str.replaceAll("([0-9])\\.0+([^0-9]|$)", "$1$2");

        retStr = str;

        if (negative) {
            retStr = "-"+retStr;
        }

        return retStr;

    }

    static boolean isNumeric(String str) {
        try {
            Double.parseDouble(str);
        } catch (NumberFormatException nfe) {
            return false;
        }
        return true;
    }

}