/* examples/sigpipe.c */

static void
catcher (int sig)
{
  /* Ignore the signal */
}


static void
ignore_sigpipe (void)
{
  struct sigaction oldsig;
  struct sigaction sig;

  sig.sa_handler = &catcher;
  sigemptyset (&sig.sa_mask);
#ifdef SA_INTERRUPT
  sig.sa_flags = SA_INTERRUPT;  /* SunOS */
#else
  sig.sa_flags = SA_RESTART;
#endif
  if (0 != sigaction (SIGPIPE,
                      &sig,
                      &oldsig))
    fprintf (stderr,
             "Failed to install SIGPIPE handler: %s\n", strerror (errno));
}


int
main ()
{
  ignore_sigpipe ();
  /* Do actual application work here */
  return 0;
}
