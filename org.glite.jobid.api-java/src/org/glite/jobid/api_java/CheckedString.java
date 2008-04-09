package org.glite.jobid.api_java;

/**
 * Class which escapes \ and new line signs in string which is set as parameter
 * in constructor
 * 
 * @author Pavel Piskac (173297@mail.muni.cz)
 * @version 15. 3. 2008
 */
public class CheckedString {
    
    String checkedString;
    
    /**
     * Creates new instance of CheckedString.
     * 
     * @param checkedString string which will be converted
     * @throws java.lang.IllegalArgumentException if checkedString is null
     */
    public CheckedString(String checkedString) {
        if (checkedString == null) {
            throw new IllegalArgumentException("checkedString is null");
        }
        
        checkedString = checkedString.replaceAll("[\\\"]", "\\\\\"");
        checkedString = checkedString.replaceAll("[\n]", "\\\\\\\\n");
        this.checkedString = checkedString;
    }

    /**
     * Gets converted string.
     * 
     * @return converted string
     */
    public String getCheckedString() {
        return checkedString;
    }

    /**
     * Sets string which will be converted.
     * 
     * @param checkedString string which will be converted.
     */
    public void setCheckedString(String checkedString) {
        checkedString = checkedString.replaceAll("[\\\"]", "\\\\\"");
        checkedString = checkedString.replaceAll("[\n]", "\\\\\\\\n");
        this.checkedString = checkedString;
    }
    
    /**
     * Returns converted string.
     * 
     * @return converted string
     */
    public String toString() {
        return checkedString;
    }

}
