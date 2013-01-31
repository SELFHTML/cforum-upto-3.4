package Plugins::UserconfCategories;

# \file UserconfCategories.pm
# \author Christian Kruse, <cjk@wwwtech.de>
#
# a plugin to make whitelist and blacklist in fo_userconf nicer

# {{{ initial comments
#
# $LastChangedDate: 2009-01-16 14:32:24 +0100 (Fri, 16 Jan 2009) $
# $LastChangedRevision: 1639 $
# $LastChangedBy: ckruse $
#
# }}}

# {{{ program headers
#
use strict;

sub VERSION {(q$Revision: 1639 $ =~ /([\d.]+)\s*$/)[0] or '0.0'}

use CGI::Carp qw(fatalsToBrowser);

use ForumUtils qw/get_conf_val/;
# }}}

push @{$main::Plugins->{display}},\&execute;
push @{$main::Plugins->{writeconf}},\&transform;

sub transform {
  my ($fo_default_conf,$fo_view_conf,$fo_userconf_conf,$user_config,$own_ucfg,$cgi) = @_;

  $own_ucfg->{global}->{WhiteList}->[0]->[0] =~ s!\015\012|\015|\012!,!sg;
  $own_ucfg->{global}->{BlackList}->[0]->[0] =~ s!\015\012|\015|\012!,!sg;
}

sub execute {
  my ($fo_default_conf,$fo_userconf_conf,$user_config,$cgi,$tpl) = @_;

  my $cats = get_conf_val($user_config,'global','ShowCategories');

  if($cats) {
    my $catsvar = new CForum::Template::cf_tpl_variable_t($CForum::Template::TPL_VARIABLE_ARRAY);
    my @catsary = split /,/,$cats;

    $catsvar->addvalue($_) foreach @catsary;
    $tpl->setvar('hidedcats',$catsvar);
  }

  $cats = get_conf_val($user_config,'global','HighlightCategories');
  if($cats) {
    my $catsvar = new CForum::Template::cf_tpl_variable_t($CForum::Template::TPL_VARIABLE_ARRAY);
    my @catsary = split /,/,$cats;

    $catsvar->addvalue($_) foreach @catsary;
    $tpl->setvar('highcats',$catsvar);
  }

  my $blacklist = get_conf_val($user_config,'global','BlackList');
  if($blacklist) {
    $blacklist =~ s!,!\n!g;
    $tpl->setvalue('blacklist',$blacklist);
  }

  my $whitelist = get_conf_val($user_config,'global','WhiteList');
  if($whitelist) {
    $whitelist =~ s!,!\n!g;
    $tpl->setvalue('whitelst',$whitelist);
  }


  return 0;
}

1;
# eof
