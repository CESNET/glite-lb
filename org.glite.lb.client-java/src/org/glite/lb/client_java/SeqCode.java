package org.glite.lb.client_java;

/**
 *
 * @author xpiskac
 */
public class SeqCode {
    
    private int[] seqCode = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    
    public SeqCode() {
    }

    public int[] getSeqCode() {
        return seqCode;
    }

    public void setSeqCode(int[] seqCode) {
        this.seqCode = seqCode;
    }
    
    public int getPardOfSeqCode(int part) {
        if (part < 0 || part > 9) {
            throw new IllegalArgumentException("part");
        }
        
        return seqCode[part];
    }
    
    public void setPardOfSeqCode(int part, int value) {
        if (part < 0 || part > 9) {
            throw new IllegalArgumentException("part");
        }
        
        seqCode[part] = value;
    }
    
    public String toString() {        
        String tmp = Integer.toString(seqCode[0]);    
        String output = "UI=";
        output += "000000".substring(0, 6 - tmp.length ()) + tmp;
        output += ":";
        output += "NS=";
        tmp = Integer.toString(seqCode[1]);
        output += "0000000000".substring(0, 10 - tmp.length ()) + tmp;
        output += ":";
        output += "WM=";
        tmp = Integer.toString(seqCode[2]);
        output += "0000000000".substring(0, 6 - tmp.length ()) + tmp;
        output += ":";
        output += "BH=";
        tmp = Integer.toString(seqCode[3]);
        output += "0000000000".substring(0, 10 - tmp.length ()) + tmp;
        output += ":";
        output += "JSS=";
        tmp = Integer.toString(seqCode[4]);
        output += "0000000000".substring(0, 6 - tmp.length ()) + tmp;
        output += ":";
        output += "LM=";
        tmp = Integer.toString(seqCode[5]);
        output += "0000000000".substring(0, 6 - tmp.length ()) + tmp;
        output += ":";
        output += "LMRS=";
        tmp = Integer.toString(seqCode[6]);
        output += "0000000000".substring(0, 6 - tmp.length ()) + tmp;
        output += ":";
        output += "APP=";
        tmp = Integer.toString(seqCode[7]);
        output += "0000000000".substring(0, 6 - tmp.length ()) + tmp;
        output += ":";
        output += "LBS=";
        tmp = Integer.toString(seqCode[8]);
        output += "0000000000".substring(0, 6 - tmp.length ()) + tmp;
        return output;
    }
}
