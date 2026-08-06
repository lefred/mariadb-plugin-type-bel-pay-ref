package My::Suite::Type_bel_pay_ref;

@ISA = qw(My::Suite);

return "No type_bel_pay_ref plugin" unless $ENV{TYPE_BEL_PAY_REF_SO};

sub is_default { 1 }

bless { };
