from src.wire import protocol


def test_parse_envelope_roundtrips_a_valid_ack():
    raw = protocol.envelope("page.ack", protocol.PageAck(page_id="pg1"))
    env = protocol.parse_envelope(raw)
    assert env is not None
    assert env.type == "page.ack"
    ack = protocol.PageAck.model_validate(env.body)
    assert ack.page_id == "pg1"
    assert ack.by == "device"


def test_parse_envelope_rejects_non_json():
    assert protocol.parse_envelope("not json") is None


def test_parse_envelope_rejects_missing_required_fields():
    assert protocol.parse_envelope('{"type":"page.ack"}') is None
