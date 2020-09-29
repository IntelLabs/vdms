import java.util.ArrayList;
import java.util.List;

class VdmsTransaction
{
    byte size[];
    byte buffer[];
    
    public VdmsTransaction(byte[] nSize, byte nBuffer[])
    {
	size = nSize;
	buffer = nBuffer;
    }

    public byte[] GetSize()
    {
	return size;
    }

    public byte[] GetBuffer()
    {
	return buffer;
    }

}
