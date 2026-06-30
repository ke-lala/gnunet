struct MHD_Action *
simple_reply_to_request (struct MHD_Request *request)
{
  const char *data = "<html><body><p>Error!</p></body></html>";

  return MHD_action_from_response (
    request,
    MHD_response_from_buffer_static (
      MHD_HTTP_STATUS_NOT_FOUND,
      strlen (data),
      data);
    );
}
