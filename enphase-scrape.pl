#!/usr/bin/perl
#
# Scrape data from Envoy, encode it as JSON, and send it to emond via
# a 0mq socket for display.  Run from cron once per minute.
#
# Requires these Raspbian packages:
#   liblwp-protocol-https-perl
#   libzmq-dev
#   libzeromq-perl
#   libjson-perl
#
# Enphase Envoy scrape code borrowed from enphase-pvoutput.pl
#   Copyright Eric Sandeen <sandeen@sandeen.net> 2010
#   released under GNU GPL v3 or later
#
use HTTP::Request::Common qw(POST GET);
use LWP::UserAgent;
use ZeroMQ qw/:all/;
use JSON;
use strict;

my $envoy_hostname = "192.168.1.210";
my $push_addr = "ipc:///tmp/emond";

my $ua = LWP::UserAgent->new;
my $req = GET "http://$envoy_hostname/production";
my $res = $ua->request($req);

die "Couldn't get to envoy: " . $res->status_line . "\n" if (! $res->is_success);
my $raw_html = $res->content;

# We don't actually use all of these but can gather them easily...
my $current_power;
my $current_units;
my $daily_energy;
my $daily_units;
my $weekly_energy;
my $weekly_units;
my $lifetime_energy;
my $lifetime_units;

if ($raw_html =~ s!<td>Currently</td>.*<td>\s*(.*?) (.?)W</td>.*</tr>.*<td>Today</td>.*<td>\s*(.*?) (.?)Wh</td>.*</tr>.*<td>Past Week</td>.*<td>\s*(.*?) (.?)Wh</td>.*</tr>.*<td>Since Installation</td>.*<td>\s*(.*?) (.?)Wh</td>.*</tr>.*!!gsi) {
  $current_power = $1;
  $current_units = $2;
  $daily_energy = $3;
  $daily_units = $4;
  $weekly_energy = $5;
  $weekly_units = $6;
  $lifetime_energy = $7;
  $lifetime_units = $8;
} else {
  die "Couldn't get power production, something's wrong, bailing out.\n";
}

# Convert from kWh or MWh if needed
if ($current_units eq 'k') {
	$current_power *= 1000;
}
if ($current_units eq 'M') {
	$current_power *= 1000000;
}
if ($daily_units eq 'k') {
	$daily_energy *= 1000;
}
if ($daily_units eq 'M') {
	$daily_energy *= 1000000;
}
if ($weekly_units eq 'k') {
	$weekly_energy *= 1000;
}
if ($weekly_units eq 'M') {
	$weekly_energy *= 1000000;
}
if ($lifetime_units eq 'k') {
	$lifetime_energy *= 1000;
}
if ($lifetime_units eq 'M') {
	$lifetime_energy *= 1000000;
}

# Convert to JSON
my %envoy_data = (
	current_power => int $current_power,
	daily_energy => int $daily_energy,
	weekly_energy => int $weekly_energy,
	lifetime_energy => int $lifetime_energy,
);
my $json_obj = new JSON;
my $json_text = $json_obj->encode(\%envoy_data);
#printf "envoy.sample %s\n", $json_text;

# Send via 0MQ
my $ctx = ZeroMQ::Context->new;
my $sock = $ctx->socket(ZMQ_PUSH);
$sock->connect($push_addr);
$sock->send($json_text);
$sock->close();
$ctx->term();

exit;
