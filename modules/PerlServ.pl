sub do_help
{
  my ($client, $message) = @_;

  $client->reply('HELP you say?');
  Services::log($client->name, 'says ', $message);
}


my %commands = (
    'HELP' => 'do_help'
    );

Services::Service::register('PerlServ', \%commands);
