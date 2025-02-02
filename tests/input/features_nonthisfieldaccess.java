
public class features_nonthisfieldaccess {
  public features_nonthisfieldaccess() {}
  public int x;
  public void m() {
    new features_nonthisfieldaccess().x = 42;
  }
}

